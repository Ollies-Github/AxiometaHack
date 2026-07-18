import os
import re

import modal


app = modal.App("hatos-ai")
image = modal.Image.debian_slim().pip_install(
    "fastapi>=0.115",
    "httpx>=0.28",
    "pydantic>=2.10",
)


def compact(text: str, limit: int) -> str:
    """Normalize prose into a predictable display-sized string."""
    clean = re.sub(r"\s+", " ", text).strip()
    if len(clean) <= limit:
        return clean
    clipped = clean[: limit - 1].rsplit(" ", 1)[0]
    return f"{clipped}…"


@app.function(
    image=image,
    secrets=[modal.Secret.from_name("linkup-secret")],
    timeout=35,
)
@modal.fastapi_endpoint(method="POST")
def ask(payload: dict) -> dict:
    """Return a sourced web answer and two strings suitable for 160x80 screens."""
    import httpx
    from fastapi import HTTPException

    question = str(payload.get("question", "")).strip()
    if not question:
        raise HTTPException(status_code=400, detail="question is required")

    response = httpx.post(
        "https://api.linkup.so/v1/search",
        headers={"Authorization": f"Bearer {os.environ['LINKUP_API_KEY']}"},
        json={
            "q": question,
            "depth": "fast",
            "outputType": "sourcedAnswer",
            "includeInlineCitations": True,
        },
        timeout=25,
    )
    response.raise_for_status()
    result = response.json()
    answer = result.get("answer") or result.get("sourcedAnswer") or ""

    if not answer:
        candidates = result.get("results", [])
        answer = candidates[0].get("content", "No answer found") if candidates else "No answer found"

    return {
        "question": question,
        "answer": answer,
        "wearerText": compact(answer, 90),
        "outwardText": compact(f"Asked: {question}", 48),
        "sources": result.get("sources", result.get("results", [])),
    }
