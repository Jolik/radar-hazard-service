from datetime import datetime, timezone

from fastapi.testclient import TestClient

from radar_hazard_service.main import create_app


def test_health_endpoint_returns_ok():
    app = create_app()
    client = TestClient(app)

    response = client.get("/health")

    assert response.status_code == 200
    assert response.json() == {"status": "ok"}


def test_hazard_endpoint_returns_assessment():
    app = create_app()
    client = TestClient(app)

    payload = {
        "returns": [
            {
                "timestamp": datetime(2024, 3, 26, tzinfo=timezone.utc).isoformat(),
                "distance_m": 200,
                "radial_velocity_ms": 30,
                "intensity_dbz": 50,
            }
        ],
        "distance_threshold_m": 400,
        "velocity_threshold_ms": 20,
    }

    response = client.post("/hazard", json=payload)

    assert response.status_code == 200
    body = response.json()
    assert body["hazard_level"] in {"caution", "danger"}
    assert body["score"] >= 0
    assert body["dominant_return"]["distance_m"] == 200.0
