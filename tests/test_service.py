from datetime import datetime, timezone

from radar_hazard_service.models import HazardAssessmentRequest, RadarReturn
from radar_hazard_service.service import RadarHazardScorer


def _sample_return(**overrides):
    data = {
        "timestamp": datetime(2024, 3, 26, tzinfo=timezone.utc),
        "distance_m": 400.0,
        "radial_velocity_ms": 10.0,
        "intensity_dbz": 30.0,
    }
    data.update(overrides)
    return RadarReturn(**data)


def test_safe_level_when_returns_far_and_slow():
    scorer = RadarHazardScorer()
    request = HazardAssessmentRequest(returns=[_sample_return(distance_m=800, radial_velocity_ms=5)])

    response = scorer.assess(request)

    assert response.hazard_level.value == "safe"
    assert response.score < 1.2


def test_caution_level_when_thresholds_exceeded():
    scorer = RadarHazardScorer()
    request = HazardAssessmentRequest(returns=[_sample_return(distance_m=300, radial_velocity_ms=28)])

    response = scorer.assess(request)

    assert response.hazard_level.value == "caution"
    assert 1.2 <= response.score < 2.5


def test_danger_level_when_close_and_fast():
    scorer = RadarHazardScorer()
    request = HazardAssessmentRequest(
        returns=[_sample_return(distance_m=100, radial_velocity_ms=35, intensity_dbz=55)]
    )

    response = scorer.assess(request)

    assert response.hazard_level.value == "danger"
    assert response.score >= 2.5
