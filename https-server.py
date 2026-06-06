from http.server import ThreadingHTTPServer, SimpleHTTPRequestHandler
import ssl

HOST = "0.0.0.0"
PORT = 8765
CERT_FILE = "certs/santron626-local.crt"
KEY_FILE = "certs/santron626-local.key"


class NoCacheHandler(SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header("Cache-Control", "no-store, max-age=0")
        super().end_headers()


httpd = ThreadingHTTPServer((HOST, PORT), NoCacheHandler)
context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
context.load_cert_chain(certfile=CERT_FILE, keyfile=KEY_FILE)
httpd.socket = context.wrap_socket(httpd.socket, server_side=True)

print(f"Serving HTTPS on https://192.168.2.122:{PORT}")
httpd.serve_forever()
