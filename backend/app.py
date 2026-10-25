"""
Flask Application Entry Point.
Serves both REST API and Frontend static dashboard assets.
"""

import os
from flask import Flask, send_from_directory
from flask_cors import CORS
from routes import api_bp

PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FRONTEND_DIR = os.path.join(PROJECT_ROOT, "frontend")

def create_app():
    app = Flask(__name__, static_folder=FRONTEND_DIR, static_url_path="")
    CORS(app)
    
    app.register_blueprint(api_bp)
    
    @app.route("/")
    def index():
        return send_from_directory(FRONTEND_DIR, "index.html")
        
    @app.route("/<path:path>")
    def static_proxy(path):
        file_path = os.path.join(FRONTEND_DIR, path)
        if os.path.exists(file_path):
            return send_from_directory(FRONTEND_DIR, path)
        return send_from_directory(FRONTEND_DIR, "index.html")
        
    return app

if __name__ == "__main__":
    app = create_app()
    port = int(os.environ.get("PORT", 5001))
    print(f"Starting Intelligent CPU Scheduling Platform on http://127.0.0.1:{port}")
    app.run(host="127.0.0.1", port=port, debug=False)
