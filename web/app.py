from pathlib import Path
import json
import re
import subprocess

from flask import Flask, jsonify, render_template, request

PROJECT_ROOT = Path(__file__).resolve().parent.parent
C_CORE = PROJECT_ROOT / "museum_core"

app = Flask(__name__)


def run_c_command(*args):
    """Execute a command through the C application core."""
    if not C_CORE.exists():
        return {
            "success": False,
            "error": "C core executable not found. Run: make -C c_core"
        }, 500

    try:
        result = subprocess.run(
            [str(C_CORE), *args],
            cwd=PROJECT_ROOT,
            capture_output=True,
            text=True,
            timeout=5
        )
    except subprocess.TimeoutExpired:
        return {
            "success": False,
            "error": "C core request timed out."
        }, 504

    if result.returncode != 0:
        return {
            "success": False,
            "error": result.stderr.strip() or "C core command failed."
        }, 400

    lines = [line.strip() for line in result.stdout.splitlines() if line.strip()]

    if not lines:
        return {
            "success": False,
            "error": "C core returned no data."
        }, 500

    try:
        return json.loads(lines[-1]), 200
    except json.JSONDecodeError:
        return {
            "success": False,
            "error": "Invalid response from C core.",
            "raw": result.stdout.strip()
        }, 500


def parse_exhibits(output):
    """Parse the C --list table into JSON."""
    exhibits = []

    for line in output.splitlines():
        line = line.strip()

        match = re.match(
            r"^(\d+)\s+(.+?)\s{2,}(.+?)\s{2,}(\d+)$",
            line
        )

        if match:
            exhibits.append({
                "id": int(match.group(1)),
                "name": match.group(2).strip(),
                "category": match.group(3).strip(),
                "visitors": int(match.group(4))
            })

    return exhibits


def parse_tickets(output):
    """Parse the C --tickets table into JSON."""
    tickets = []

    for line in output.splitlines():
        line = line.strip()

        match = re.match(
            r"^(\d+)\s+(\d+)\s+(.+?)\s+\$([0-9]+\.[0-9]{2})$",
            line
        )

        if match:
            tickets.append({
                "id": int(match.group(1)),
                "exhibit_id": int(match.group(2)),
                "visitor_name": match.group(3).strip(),
                "price": float(match.group(4))
            })

    return tickets


@app.get("/")
def index():
    return render_template("index.html")


@app.get("/api/dashboard")
def dashboard():
    data, status = run_c_command("--dashboard")
    return jsonify(data), status


@app.get("/api/exhibits")
def list_exhibits():
    try:
        result = subprocess.run(
            [str(C_CORE), "--list"],
            cwd=PROJECT_ROOT,
            capture_output=True,
            text=True,
            timeout=5
        )

        if result.returncode != 0:
            return jsonify({
                "success": False,
                "error": result.stderr.strip()
            }), 400

        return jsonify({
            "success": True,
            "exhibits": parse_exhibits(result.stdout)
        })

    except subprocess.TimeoutExpired:
        return jsonify({
            "success": False,
            "error": "C core request timed out."
        }), 504


@app.get("/api/exhibits/<int:exhibit_id>")
def search_exhibit(exhibit_id):
    data, status = run_c_command("--search", str(exhibit_id))
    return jsonify(data), status


@app.post("/api/exhibits")
def add_exhibit():
    data = request.get_json(silent=True) or {}

    name = str(data.get("name", "")).strip()
    category = str(data.get("category", "")).strip()

    if not name or not category:
        return jsonify({
            "success": False,
            "error": "Name and category are required."
        }), 400

    result, status = run_c_command(
        "--add-exhibit",
        name,
        category
    )

    return jsonify(result), status


@app.delete("/api/exhibits/<int:exhibit_id>")
def delete_exhibit(exhibit_id):
    result, status = run_c_command(
        "--delete",
        str(exhibit_id)
    )

    return jsonify(result), status


@app.get("/api/tickets")
def list_tickets():
    try:
        result = subprocess.run(
            [str(C_CORE), "--tickets"],
            cwd=PROJECT_ROOT,
            capture_output=True,
            text=True,
            timeout=5
        )

        if result.returncode != 0:
            return jsonify({
                "success": False,
                "error": result.stderr.strip()
            }), 400

        return jsonify({
            "success": True,
            "tickets": parse_tickets(result.stdout)
        })

    except subprocess.TimeoutExpired:
        return jsonify({
            "success": False,
            "error": "C core request timed out."
        }), 504


@app.post("/api/tickets")
def purchase_ticket():
    data = request.get_json(silent=True) or {}

    exhibit_id = data.get("exhibit_id")
    visitor_name = str(data.get("visitor_name", "")).strip()
    price = data.get("price")

    if exhibit_id is None:
        return jsonify({
            "success": False,
            "error": "Exhibit ID is required."
        }), 400

    if not visitor_name:
        return jsonify({
            "success": False,
            "error": "Visitor name is required."
        }), 400

    if price is None:
        return jsonify({
            "success": False,
            "error": "Ticket price is required."
        }), 400

    try:
        exhibit_id = int(exhibit_id)
        price = float(price)
    except (TypeError, ValueError):
        return jsonify({
            "success": False,
            "error": "Exhibit ID and price must be valid numbers."
        }), 400

    if exhibit_id < 1 or price <= 0:
        return jsonify({
            "success": False,
            "error": "Invalid exhibit ID or ticket price."
        }), 400

    result, status = run_c_command(
        "--purchase",
        str(exhibit_id),
        visitor_name,
        str(price)
    )

    return jsonify(result), status


@app.get("/api/health")
def health():
    return jsonify({
        "status": "ok",
        "c_core": C_CORE.exists()
    })


@app.errorhandler(404)
def not_found(error):
    if request.path.startswith("/api/"):
        return jsonify({
            "success": False,
            "error": "API endpoint not found."
        }), 404

    return error


@app.errorhandler(405)
def method_not_allowed(error):
    if request.path.startswith("/api/"):
        return jsonify({
            "success": False,
            "error": "HTTP method not allowed."
        }), 405

    return error


if __name__ == "__main__":
    app.run(
        host="127.0.0.1",
        port=5000,
        debug=True
    )
EOF