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


# @app.get("/episodes/{episode_id}/download")
# def download_episode(
#     episode_id: int):

#     return FileResponse(
#         f"episodes/{episode_id}.zip"
#     )


@app.get("/episodes/{episode_id}/download")
def download_episode(episode_id: int):
    episode_dir = f"episodes/{episode_id}"
    
    if not os.path.exists(episode_dir):
        raise HTTPException(status_code=404, detail="Episode not found")
    
    # Creates episodes/1.zip from the folder episodes/1/
    zip_path = shutil.make_archive(
        base_name=f"episodes/{episode_id}",  # output path without extension
        format="zip",
        root_dir="episodes",                  # zip relative to this folder
        base_dir=str(episode_id)             # which subfolder to zip
    )

    return FileResponse(
        path=zip_path,
        media_type="application/zip",
        filename=f"episode_{episode_id}.zip"
    )
#Task 4 
#Run->
#     uvicorn RestAPI:app --reload