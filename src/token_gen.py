import os
import webbrowser
from flask import Flask, request
from dotenv import load_dotenv
import serial

load_dotenv()
client_id = os.getenv("CLIENT_ID")

app = Flask(__name__)
if __name__ == "__main__":
    app.run(port=8080)

@app.route("/callback")
def callback():
    # To access params in the url
    # that gets the auth code
    code = request.args.get("code")
    ser = serial.Serial("/dev/ttyACM0", 115200)
    byte_code = code.encode('utf-8')
    print(byte_code)
    ser.write(byte_code)
    ser.close()
    return "<center><h1>You can close this page now</h1></center>"


def main():
    redirect_uri = "http://localhost:8080/callback"

    auth = "https://accounts.spotify.com/authorize?"

    scope = "app-remote-control playlist-modify-public playlist-modify-private streaming user-read-playback-state user-modify-playback-state"

    params = "client_id=" + client_id + "&response_type=" + 'code' + "&scope=" + scope + "&redirect_uri="+ redirect_uri
    webbrowser.open(auth+params)


main()