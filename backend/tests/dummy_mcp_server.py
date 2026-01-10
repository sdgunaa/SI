import sys
import json
import os

def log(msg):
    sys.stderr.write(f"[DummyServer] {msg}\n")
    sys.stderr.flush()

log("Starting dummy server")

while True:
    try:
        line = sys.stdin.readline()
        if not line:
            break
        
        line = line.strip()
        if not line:
            continue

        try:
            req = json.loads(line)
        except json.JSONDecodeError:
            log("Invalid JSON")
            continue
            
        log(f"Received: {line}")
        
        method = req.get("method")
        msg_id = req.get("id")
        
        if msg_id is not None:
            resp = {"jsonrpc": "2.0", "id": msg_id}
            
            if method == "initialize":
                resp["result"] = {
                    "protocolVersion": "0.1.0", 
                    "capabilities": {}, 
                    "serverInfo": {"name": "dummy", "version": "1.0"}
                }
            elif method == "tools/list":
                resp["result"] = {
                    "tools": [{
                        "name": "echo", 
                        "description": "echoes back", 
                        "inputSchema": {
                            "type": "object", 
                            "properties": { "text": { "type": "string" } }
                        }
                    }]
                }
            elif method == "tools/call":
                params = req.get("params", {})
                args = params.get("arguments", {})
                resp["result"] = {
                    "content": [{
                        "type": "text", 
                        "text": f"Echo: {args.get('text', '')}"
                    }]
                }
            else:
                 resp["error"] = {"code": -32601, "message": "Method not found"}
            
            out_msg = json.dumps(resp)
            sys.stdout.write(out_msg + "\n")
            sys.stdout.flush()
            
    except Exception as e:
        log(f"Error loop: {e}")
