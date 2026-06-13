from fastapi import FastAPI
from pathlib import Path
import json

from fastapi.responses import FileResponse

app = FastAPI()

EPISODES = Path("episodes")

@app.get("/episodes")
def list_episodes():

    return [
        p.name
        for p in EPISODES.iterdir()
        if p.is_dir()
    ]

@app.get("/episodes/{episode_id}")
def get_episode_metadata(
    episode_id: str):
    with open(
        EPISODES /
        str(episode_id) /
        "metadata.json"
    ) as f:

        return json.load(f)


@app.get("/episodes/{episode_id}/download")
def download_episode(
    episode_id: int):

    return FileResponse(
        f"episodes/{episode_id}.zip"
    )


#Task 4 
#Run->
#     uvicorn RestAPI:app --reload