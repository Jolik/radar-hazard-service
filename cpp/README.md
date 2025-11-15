# Radar Hazard C++ Pipeline

This directory contains a standalone C++ implementation of the radar hazard contour generation pipeline described in the BUFR-based methodology. The executable performs the following steps:

1. **BUFR ingestion** – reads raw BUFR files and decodes them using configurable descriptor tables.
2. **Geodesic cell reconstruction** – computes geographic coordinates for the radar gate centre and vertices using the formulas from the methodology.
3. **8-connected clustering** – groups neighbouring grid cells into contiguous storm clusters.
4. **Echo top fusion** – adds ECHO TOP heights from a supplementary matrix to each cell and carries the peak value through cluster aggregation.
5. **Radar merger** – unites overlapping polygons with identical phenomenon codes using boolean geometry.
6. **Filtering** – applies configurable reflectivity thresholds and phenomenon whitelists prior to clustering.
7. **Map rendering** – paints merged contours into a bitmap image for visual inspection.

## Building

```bash
cd cpp
cmake -S . -B build
cmake --build build
```

## Running

```bash
./build/radar_hazard_app <path-to-config.json>
```

A sample configuration (`data/sample_config.json`) and descriptor table (`data/descriptor_tables.json`) are provided to illustrate the expected structure. The configuration controls optional bitmap output through the `image_output_path`, `image_width`, and `image_height` fields.
