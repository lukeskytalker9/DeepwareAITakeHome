from fastapi import FastAPI
from fastapi.responses import HTMLResponse
import random



app = FastAPI()

recording = False
episode_count = 3


@app.get("/")
def dashboard():

    with open("Webpages/index.html", "r") as f:
        html = f.read()

    return HTMLResponse(content=html)


@app.get("/state")
def get_state():

    joints = []

    for i in range(6):
        joints.append({
            "id": i,
            "position": round(
                random.uniform(-3.14, 3.14),
                3
            ),
            "velocity": round(
                random.uniform(-1.0, 1.0),
                3
            ),
            "torque": round(
                random.uniform(-5.0, 5.0),
                3
            )
        })

    return {
        "recording": recording,
        "joints": joints
    }


@app.get("/episodes")
def get_episodes():

    return {
        "count": episode_count
    }


@app.post("/start")
def start_recording():

    global recording

    recording = True

    return {
        "status": "recording started"
    }


@app.post("/stop")
def stop_recording():

    global recording
    global episode_count

    recording = False

    episode_count += 1

    return {
        "status": "recording stopped"
    }


    #Run->
#     uvicorn RestAPIWithDashboard:app --reload