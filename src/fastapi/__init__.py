"""A minimal FastAPI-compatible shim for offline testing."""

from __future__ import annotations

import asyncio
import inspect
from dataclasses import dataclass
from typing import Any, Awaitable, Callable, Dict, Tuple, get_type_hints


RouteHandler = Callable[..., Awaitable[Any] | Any]


@dataclass
class RegisteredRoute:
    method: str
    path: str
    handler: RouteHandler


class FastAPI:
    """Simplified FastAPI-like interface used for unit testing."""

    def __init__(self, title: str, version: str) -> None:
        self.title = title
        self.version = version
        self._routes: Dict[Tuple[str, str], RegisteredRoute] = {}

    def get(self, path: str, *, tags: list[str] | None = None) -> Callable[[RouteHandler], RouteHandler]:
        return self._register("GET", path)

    def post(
        self,
        path: str,
        *,
        response_model: type[Any] | None = None,
        tags: list[str] | None = None,
    ) -> Callable[[RouteHandler], RouteHandler]:
        return self._register("POST", path)

    def _register(self, method: str, path: str) -> Callable[[RouteHandler], RouteHandler]:
        def decorator(func: RouteHandler) -> RouteHandler:
            key = (method.upper(), path)
            self._routes[key] = RegisteredRoute(method=method.upper(), path=path, handler=func)
            return func

        return decorator

    def get_route(self, method: str, path: str) -> RegisteredRoute:
        try:
            return self._routes[(method.upper(), path)]
        except KeyError as exc:
            raise KeyError(f"No route registered for {method} {path}") from exc


class Response:
    def __init__(self, status_code: int, payload: Any) -> None:
        self.status_code = status_code
        self._payload = payload

    def json(self) -> Any:
        return self._payload


class TestClient:
    """Very small HTTP-like client for the shimmed FastAPI application."""

    def __init__(self, app: FastAPI) -> None:
        self.app = app

    def get(self, path: str) -> Response:
        return self._call("GET", path)

    def post(self, path: str, json: Any | None = None) -> Response:
        return self._call("POST", path, json=json)

    def _call(self, method: str, path: str, json: Any | None = None) -> Response:
        route = self.app.get_route(method, path)
        handler = route.handler
        kwargs: Dict[str, Any] = {}

        signature = inspect.signature(handler)
        type_hints = get_type_hints(handler)
        if json is not None and signature.parameters:
            param = next(iter(signature.parameters.values()))
            annotation = type_hints.get(param.name, param.annotation)
            if hasattr(annotation, "model_validate"):
                kwargs[param.name] = annotation.model_validate(json)
            else:
                kwargs[param.name] = json

        result = handler(**kwargs)
        if inspect.iscoroutine(result):
            result = asyncio.run(result)

        if hasattr(result, "model_dump"):
            payload = result.model_dump()
        else:
            payload = result

        return Response(status_code=200, payload=payload)


TestClient.__test__ = False  # prevent pytest from collecting the helper class as a test


__all__ = ["FastAPI", "TestClient"]
