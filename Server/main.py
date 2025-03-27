from flask import Flask, request, send_file,Response, jsonify
from tv_src import *
import requests
import io
import json
import threading
import opencc
import time
import threading
from binance_api import *


do_update = False
converter = opencc.OpenCC('t2s.json')  # 繁体转简体
app = Flask(__name__)
model = "gpt-4o-mini"
api_key = "YOURKEY"
messages = [
    {
        "role": "system",
        "content": "你是一个有用的助理，尽可能给出最短最短并且最有用的回答。"
    }
]

global_answer = {}

def get_openai_completion(messages_in):
    url = "https://api.xty.app/v1/chat/completions"
    req_msg = messages.copy()  # 创建一个列表的副本，避免直接修改原始列表
    req_msg.append({"role": "user", "content": messages_in})
    headers = {
        "Content-Type": "application/json",
        "Authorization": f"Bearer {api_key}"
    }
    data = {
        "model": model,
        "messages": req_msg
    }
    response = requests.post(url, headers=headers, data=json.dumps(data))
    return response.json().get("choices")[0].get("message").get('content')


def speech2text(data):
    url = "https://api.xty.app/v1/audio/transcriptions"
    headers = {
        "Authorization": f"Bearer {api_key}"
    }

    file_obj = io.BytesIO(data)

    files = {
        "file": ("audio.wav", file_obj, "audio/wav")
    }
    data = {
        "model": "whisper-1",
        "language": "zh"
    }

    response = requests.post(url, headers=headers, files=files, data=data)

    if response.status_code == 200:
        cn_text = response.json().get("text")
        cn_text = converter.convert(cn_text)
        return cn_text
    else:
        return f"Error: {response.status_code} - {response.text}"

def parse_req_thread(usernum, data):
    global global_answer
    text = speech2text(data)
    answer = text + "|" + get_openai_completion(text)
    print(answer)
    global_answer[f"{usernum}"] = answer


@app.route('/getanswer', methods=['GET'])
def handle_get():
    global global_answer
    user_num = request.headers.get("mode")
    if global_answer.get(f"{user_num}") is not None:
        answer = global_answer.get(f"{user_num}")
        global_answer[f"{user_num}"] = None
        return answer
    return "WAIT!"


@app.route('/upload', methods=['POST'])
def handle_data():
    global global_answer
    data = request.data  # 获取POST请求的字节流数据
    with open('received_data.wav', 'wb') as f:
        f.write(data)
    user_num = request.headers.get("mode")
    global_answer[f"{user_num}"] = None
    my_thread = threading.Thread(target=parse_req_thread, args=(user_num, data))
    my_thread.daemon = True
    my_thread.start()
    return "Start processing"

@app.route('/firmware', methods=['GET'])
def firmware():
    with open('/mnt/disk/firmware/version.txt', 'r', encoding='utf-8') as file:
        firmware_ver = file.read()
        return firmware_ver

@app.route('/firmware.bin', methods=['GET'])
def firmware_bin():
    return send_file('/mnt/disk/firmware/firmware.bin', as_attachment=True)

@app.route('/latest_price', methods=['POST'])
def esp_req_latest_price():
    json_stuff = json.loads(request.data.decode())
    cointype = json_stuff.get("cointype")
    token = json_stuff.get("token")
    if (token == "Lynnette"):
        return get_latest_price(cointype)
    return {}


if __name__ == '__main__':
    app.run(debug=False, host='0.0.0.0', port=51156)
