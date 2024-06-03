from flask import Flask, request, jsonify, render_template

app = Flask(__name__)

sensor_data = {}

@app.route("/")
def index():
    return render_template("index.html", data=sensor_data)

@app.route("/submit", methods=["POST"])
def submit():
    global sensor_data
    data = request.get_json()
    if data and 'send_val' in data:
        sensor_data['send_val'] = data['send_val']
        print(f"Received data: {data['send_val']}")
    else:
        print("No 'send_val' found in JSON data")
    return "Data received", 200

if __name__ == "__main__":
    app.run(debug=True)
