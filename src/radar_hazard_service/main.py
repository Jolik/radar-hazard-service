"""FastAPI entrypoints for the radar hazard service."""

from __future__ import annotations

from fastapi import FastAPI

from .models import HazardAssessmentRequest, HazardAssessmentResponse
from .service import RadarHazardScorer


def create_app() -> FastAPI:
    """Create and configure the FastAPI application."""

    app = FastAPI(title="Radar Hazard Service", version="0.1.0")
    scorer = RadarHazardScorer()

    @app.get("/health", tags=["health"])
    async def health() -> dict[str, str]:
        return {"status": "ok"}

    @app.post("/hazard", response_model=HazardAssessmentResponse, tags=["hazard"])
    async def hazard_assessment(request: HazardAssessmentRequest) -> HazardAssessmentResponse:
        return scorer.assess(request)

    return app


def run() -> None:
    """Entry point for `python -m radar_hazard_service`."""

    import uvicorn

    uvicorn.run("radar_hazard_service.main:create_app", factory=True, host="0.0.0.0", port=8000)


app = create_app()
