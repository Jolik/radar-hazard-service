# Radar Hazard Service

A lightweight FastAPI microservice that classifies hazard levels from radar returns.

## Features

- `/health` endpoint for readiness checks.
- `/hazard` endpoint that accepts radar returns and thresholds to compute a hazard score.
- Simple scoring heuristics that consider distance, velocity, and intensity.

## Getting Started

1. Create a virtual environment and install dependencies:

   ```bash
   python -m venv .venv
   source .venv/bin/activate
   pip install -e .[dev]
   ```

2. Run the service locally:

   ```bash
   uvicorn radar_hazard_service.main:create_app --factory --reload
   ```

3. Execute the automated tests:

   ```bash
   pytest
   ```

## API

### `POST /hazard`

Example request:

```json
{
  "returns": [
    {
      "timestamp": "2024-03-26T12:00:00Z",
      "distance_m": 250,
      "radial_velocity_ms": 12,
      "intensity_dbz": 45
    }
  ],
  "distance_threshold_m": 500,
  "velocity_threshold_ms": 25
}
```

Example response:

```json
{
  "hazard_level": "caution",
  "score": 1.62,
  "dominant_return": {
    "timestamp": "2024-03-26T12:00:00+00:00",
    "distance_m": 250.0,
    "radial_velocity_ms": 12.0,
    "intensity_dbz": 45.0
  }
}
```

## License

MIT
