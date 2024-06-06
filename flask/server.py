from flask import Flask, request, render_template, jsonify
import datetime

app = Flask(__name__)

sensor_data = []

@app.route("/")
def index():
    return render_template("index.html")

@app.route("/submit", methods=["POST"])
def submit():
    global sensor_data
    data = request.get_json()
    if data and 'send_val' in data:
        timestamp = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        sensor_data.append({'timestamp': timestamp, 'data': data['send_val']})
        print(f"Received data: {data['send_val']} at {timestamp}")
    else:
        print("No 'send_val' found in JSON data")
    return "Data received", 200

@app.route("/data", methods=["GET"])
def get_data():
    return jsonify(sensor_data)

if __name__ == "__main__":
    app.run(debug=True)
