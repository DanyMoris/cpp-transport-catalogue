# Transport Catalogue

A C++20 console application that stores a public-transport network, answers statistical queries, draws an SVG map, and computes the fastest route between two stops.

The program reads a JSON document from `stdin` and writes a JSON response to `stdout`. It is self-contained: the JSON parser, SVG renderer, graph library, and routing engine are implemented in-project with the C++ standard library only.

This is the final project of the [Yandex Practicum](https://practicum.yandex.com/) C++ Developer course (sprints 9–13).

## Features

- **Catalogue** — add stops with geographic coordinates and road distances; add circular and linear bus routes
- **Statistics** — bus info (stop count, unique stops, road length, curvature) and the set of buses serving a stop
- **Map rendering** — project geographic coordinates onto a 2D plane and emit a layered SVG map (routes, labels, stop markers)
- **Routing** — find a minimum-time itinerary that accounts for waiting at a stop and riding one or more buses

## Tech stack

| Area | Choice |
| --- | --- |
| Language | C++20 (`std::string_view`, `std::optional`, `std::variant`, `std::unique_ptr`, `<numbers>`) |
| Dependencies | None — standard library only |
| I/O format | JSON over stdin / stdout |
| Graphics | Custom SVG 1.1 emitter (`Circle`, `Polyline`, `Text`) |
| Routing | Directed weighted graph + Floyd–Warshall all-pairs shortest paths |
| Geometry | Spherical distance on the WGS-84 sphere (Earth radius 6 371 km) |

## Architecture

The application is split into independent layers. `main` wires them together; no layer talks to I/O except `JsonReader`.

```
stdin JSON
    │
    ▼
┌─────────────┐     ┌──────────────────────┐     ┌─────────────────┐
│ JsonReader  │────▶│ TransportCatalogue   │◀────│ RequestHandler  │
│ + Builder   │     │ (stops, buses, dist) │     │ (facade)        │
└─────────────┘     └──────────────────────┘     └────────┬────────┘
      │                                                   │
      │              ┌──────────────────┐                 │
      └─────────────▶│ MapRenderer      │◀────────────────┤
                     │ + SphereProjector│                 │
                     │ + svg::*         │                 │
                     └──────────────────┘                 │
                                                          │
                     ┌──────────────────┐                 │
                     │ TransportRouter  │◀────────────────┘
                     │ graph + Router   │
                     └──────────────────┘
                              │
                              ▼
                         stdout JSON
```

| Module | Role |
| --- | --- |
| `domain` | Value types: `Stop`, `Bus`, `BusInfo` |
| `transport_catalogue` | In-memory store with pointer-stable `std::deque` storage and `string_view` indexes |
| `json` / `json_builder` | Recursive-descent JSON parser and a type-safe fluent builder |
| `svg` | Object-oriented SVG document model |
| `map_renderer` | Geographic projection and layered map composition |
| `graph` / `router` | Generic directed weighted graph and Floyd–Warshall router |
| `transport_router` | Models wait/board vertices and bus spans as graph edges |
| `request_handler` | Facade over catalogue, renderer, and router |
| `json_reader` | Parses input, dispatches `Bus` / `Stop` / `Map` / `Route` requests |

## Practices and patterns

**Design**

- **Layered architecture** — domain, storage, I/O, rendering, and routing are isolated behind small public headers
- **Facade** — `RequestHandler` is the single query API used by the JSON layer
- **Builder + type-state** — `json::Builder` returns context types (`DictItemContext`, `DictKeyContext`, `ArrayItemContext`) so illegal sequences such as `Key` inside an array fail at compile time
- **CRTP** — `svg::PathProps<Owner>` implements a fluent style API (`SetFillColor`, `SetStrokeWidth`, …) without virtual overhead
- **Visitor** — `std::visit` prints `svg::Color`, which is a `std::variant` of none / name / RGB / RGBA
- **Polymorphism + RAII** — SVG primitives inherit `Object` and are stored as `std::unique_ptr<Object>` in the document

**C++ techniques**

- **Pointer-stable storage** — stops and buses live in `std::deque`, so indexes of `string_view` and raw pointers stay valid
- **Zero-copy lookups** — `unordered_map<string_view, …>` keys point into the stored `std::string` names
- **Custom hasher** — road distances are keyed by `pair<const Stop*, const Stop*>`
- **`std::optional`** — missing buses, stops, or routes are reported without sentinel values or exceptions on the query path
- **Const-correctness and namespaces** — each subsystem has its own namespace (`domain`, `json`, `svg`, `graph`, `renderer`, `transport_router`)
- **Header / implementation split** — templates stay in headers; the rest is compiled separately

**Algorithms**

- **Floyd–Warshall** — precomputes shortest paths so each `Route` query is a reconstruction from the predecessor table
- **Two vertices per stop** — a wait vertex and a board vertex, so waiting time is a first-class edge
- **Span edges** — every pair of stops on a route becomes a bus edge with accumulated distance converted to minutes
- **Sphere projection** — longitude/latitude are scaled into the render viewport with padding and a uniform zoom coefficient

## Request types

Input is a single JSON object. Typical top-level keys:

- `base_requests` — `Stop` and `Bus` records that populate the catalogue
- `render_settings` — canvas size, colors, fonts, and palette for the map
- `routing_settings` — `bus_wait_time` (minutes) and `bus_velocity` (km/h)
- `stat_requests` — queries to answer

| `type` | Meaning | Response highlights |
| --- | --- | --- |
| `Bus` | Statistics for a route | `stop_count`, `unique_stop_count`, `route_length`, `curvature` |
| `Stop` | Buses that pass through a stop | sorted `buses` array |
| `Map` | SVG drawing of the network | `map` string with an SVG document |
| `Route` | Fastest path between two stops | `total_time` and `Wait` / `Bus` items |

Unknown names produce `{ "request_id": …, "error_message": "not found" }`.

### Minimal example

**Input**

```json
{
  "base_requests": [
    {
      "type": "Stop",
      "name": "A",
      "latitude": 55.611087,
      "longitude": 37.20829,
      "road_distances": { "B": 3900 }
    },
    {
      "type": "Stop",
      "name": "B",
      "latitude": 55.595884,
      "longitude": 37.209755,
      "road_distances": {}
    },
    {
      "type": "Bus",
      "name": "256",
      "is_roundtrip": true,
      "stops": ["A", "B", "A"]
    }
  ],
  "routing_settings": {
    "bus_wait_time": 6,
    "bus_velocity": 40
  },
  "stat_requests": [
    { "id": 1, "type": "Bus", "name": "256" },
    { "id": 2, "type": "Stop", "name": "A" },
    { "id": 3, "type": "Route", "from": "A", "to": "B" }
  ]
}
```

**Output (shape)**

```json
[
  {
    "request_id": 1,
    "stop_count": 3,
    "unique_stop_count": 2,
    "route_length": 7800,
    "curvature": 1.23
  },
  { "request_id": 2, "buses": ["256"] },
  {
    "request_id": 3,
    "total_time": 11.85,
    "items": [
      { "type": "Wait", "stop_name": "A", "time": 6.0 },
      { "type": "Bus", "bus": "256", "span_count": 1, "time": 5.85 }
    ]
  }
]
```

## Build and run

A C++20 compiler is required (`g++` 10+, `clang++` 10+, or MSVC 2019+). There is no third-party build dependency.

**Linux / macOS**

```bash
cd transport-catalogue
g++ -std=c++20 -O2 -o transport_catalogue *.cpp
./transport_catalogue < input.json > output.json
```

**Windows (PowerShell)**

```powershell
cd transport-catalogue
g++ -std=c++20 -O2 -o transport_catalogue.exe *.cpp
Get-Content input.json | .\transport_catalogue.exe > output.json
```

The binary is a filter: it does not take file-path arguments.

## Project layout

```
transport-catalogue/
├── main.cpp                 # stdin → catalogue → queries → stdout
├── domain.h                 # Stop, Bus, BusInfo
├── geo.h                    # Coordinates and spherical distance
├── transport_catalogue.h/.cpp
├── request_handler.h/.cpp
├── json.h/.cpp              # JSON DOM and parser
├── json_builder.h/.cpp      # Fluent, type-safe JSON construction
├── json_reader.h/.cpp       # Input loading and response building
├── svg.h/.cpp               # SVG primitives
├── map_renderer.h/.cpp      # Map composition
├── graph.h                  # DirectedWeightedGraph
├── router.h                 # Floyd–Warshall
├── ranges.h                 # Lightweight iterator range
└── transport_router.h/.cpp  # Transit graph and itinerary reconstruction
```

## License

Course project. Source is published for portfolio review.
