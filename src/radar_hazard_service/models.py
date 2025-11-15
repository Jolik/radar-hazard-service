"""Domain models for the radar hazard service."""

from __future__ import annotations

from dataclasses import asdict, dataclass, field
from datetime import datetime
from enum import Enum
from typing import Any, List


class HazardLevel(str, Enum):
    """Qualitative hazard levels derived from radar returns."""

    SAFE = "safe"
    CAUTION = "caution"
    DANGER = "danger"


@dataclass
class SerializableModel:
    """Small helper mimicking a subset of Pydantic's API."""

    def model_dump(self) -> dict[str, Any]:
        return asdict(self)

    @classmethod
    def model_validate(cls, data: dict[str, Any]) -> "SerializableModel":
        return cls(**data)


@dataclass
class RadarReturn(SerializableModel):
    """A single radar return measurement."""

    timestamp: datetime
    distance_m: float
    radial_velocity_ms: float
    intensity_dbz: float

    def __post_init__(self) -> None:
        if isinstance(self.timestamp, str):
            self.timestamp = datetime.fromisoformat(self.timestamp.replace("Z", "+00:00"))
        if self.distance_m <= 0:
            raise ValueError("distance_m must be positive")
        if self.intensity_dbz < -30 or self.intensity_dbz > 90:
            raise ValueError("intensity_dbz must be between -30 and 90 dBZ")


@dataclass
class HazardAssessmentRequest(SerializableModel):
    """Request payload for hazard assessment."""

    returns: List[RadarReturn]
    distance_threshold_m: float = 500.0
    velocity_threshold_ms: float = 25.0

    def __post_init__(self) -> None:
        self.returns = [self._coerce_return(item) for item in self.returns]
        if not self.returns:
            raise ValueError("returns must contain at least one radar return")
        if self.distance_threshold_m <= 0:
            raise ValueError("distance_threshold_m must be positive")
        if self.velocity_threshold_ms <= 0:
            raise ValueError("velocity_threshold_ms must be positive")

    @staticmethod
    def _coerce_return(item: RadarReturn | dict[str, Any]) -> RadarReturn:
        if isinstance(item, RadarReturn):
            return item
        if isinstance(item, dict):
            return RadarReturn(**item)
        raise TypeError("returns must contain RadarReturn instances or dicts")


@dataclass
class HazardAssessmentResponse(SerializableModel):
    """Response payload describing the hazard classification."""

    hazard_level: HazardLevel
    score: float = field(metadata={"ge": 0})
    dominant_return: RadarReturn | None = None

    def __post_init__(self) -> None:
        if self.score < 0:
            raise ValueError("score must be non-negative")
        if isinstance(self.dominant_return, dict):
            self.dominant_return = RadarReturn(**self.dominant_return)
