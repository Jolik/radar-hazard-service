"""Core hazard scoring logic for the radar hazard service."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Iterable

from .models import HazardAssessmentRequest, HazardAssessmentResponse, HazardLevel, RadarReturn


@dataclass
class HazardScore:
    """Simple container for hazard scoring results."""

    level: HazardLevel
    score: float
    dominant_return: RadarReturn | None


class RadarHazardScorer:
    """Evaluate radar returns to determine a hazard score."""

    intensity_weight: float = 1.2
    velocity_weight: float = 1.0
    distance_weight: float = 1.5

    def assess(self, request: HazardAssessmentRequest) -> HazardAssessmentResponse:
        """Assess hazard level from the request payload."""

        hazard_score = self._calculate_score(request.returns, request)
        return HazardAssessmentResponse(
            hazard_level=hazard_score.level,
            score=hazard_score.score,
            dominant_return=hazard_score.dominant_return,
        )

    def _calculate_score(
        self,
        returns: Iterable[RadarReturn],
        request: HazardAssessmentRequest,
    ) -> HazardScore:
        max_score = 0.0
        dominant = None

        for radar_return in returns:
            proximity_factor = max(0.0, (request.distance_threshold_m - radar_return.distance_m))
            proximity_score = proximity_factor / request.distance_threshold_m

            velocity_factor = abs(radar_return.radial_velocity_ms) / request.velocity_threshold_ms
            intensity_factor = (radar_return.intensity_dbz + 30) / 120  # normalize approx -30..90

            total = (
                proximity_score * self.distance_weight
                + velocity_factor * self.velocity_weight
                + intensity_factor * self.intensity_weight
            )

            if total > max_score:
                max_score = total
                dominant = radar_return

        return HazardScore(level=self._classify(max_score), score=max_score, dominant_return=dominant)

    @staticmethod
    def _classify(score: float) -> HazardLevel:
        if score >= 2.5:
            return HazardLevel.DANGER
        if score >= 1.2:
            return HazardLevel.CAUTION
        return HazardLevel.SAFE
