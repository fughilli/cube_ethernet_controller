import asyncio
import socket
import json

CONTROLLER_PORT = 5000
ENUM_COMMAND = b'enum\n'
BUTTON_TIMEOUT = 0.1

class ControllerState:
    def __init__(self, ip, dip, loop):
        self.ip = ip
        self.dip = dip
        self.loop = loop
        self.button_callback = None
        self._listen_task = None

    def set_lcd(self, x, y, text):
        msg = f"lcd:{x}:{y}:{text}\n".encode()
        self._send(msg)

    def set_backlights(self, states):
        payload = ':'.join(['1' if s else '0' for s in states])
        msg = f"backlight:{payload}\n".encode()
        self._send(msg)

    def register_button_callback(self, callback):
        self.button_callback = callback
        if not self._listen_task:
            self._listen_task = self.loop.create_task(self._listen_buttons())

    def _send(self, msg):
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.sendto(msg, (self.ip, CONTROLLER_PORT))
        sock.close()

    async def _listen_buttons(self):
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.bind(('', CONTROLLER_PORT))
        sock.setblocking(False)
        while True:
            try:
                data, addr = await self.loop.sock_recvfrom(sock, 1024)
                if addr[0] == self.ip:
                    try:
                        msg = json.loads(data.decode())
                        if 'buttons' in msg and self.button_callback:
                            self.button_callback(msg['buttons'])
                    except Exception:
                        pass
            except Exception:
                await asyncio.sleep(BUTTON_TIMEOUT)

class ControlPort:
    def __init__(self, base_ip='192.168.0.', start=50, end=65, port=CONTROLLER_PORT, loop=None):
        self.base_ip = base_ip
        self.start = start
        self.end = end
        self.port = port
        self.loop = loop or asyncio.get_event_loop()
        self.controllers = {}

    async def enumerate(self, timeout=2.0):
        tasks = []
        for i in range(self.start, self.end+1):
            ip = f"{self.base_ip}{i}"
            tasks.append(self._query_controller(ip, timeout/2))
        try:
            results = await asyncio.wait_for(asyncio.gather(*tasks), timeout=timeout)
        except asyncio.TimeoutError:
            results = []
        for result in results:
            if result:
                ip, dip = result
                self.controllers[ip] = ControllerState(ip, dip, self.loop)
        return self.controllers

    async def _query_controller(self, ip, timeout):
        loop = self.loop
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.setblocking(False)
        try:
            sock.sendto(ENUM_COMMAND, (ip, self.port))
            fut = loop.create_future()
            def on_response():
                try:
                    data, addr = sock.recvfrom(1024)
                    if addr[0] == ip:
                        msg = json.loads(data.decode())
                        if msg.get('type') == 'controller' and 'dip' in msg:
                            fut.set_result((ip, msg['dip']))
                        else:
                            fut.set_result(None)
                except Exception:
                    fut.set_result(None)
            loop.add_reader(sock.fileno(), on_response)
            try:
                await asyncio.wait_for(fut, timeout)
                return fut.result()
            except asyncio.TimeoutError:
                print(f"Enumeration timeout for {ip}")
                return None
            finally:
                loop.remove_reader(sock.fileno())
        finally:
            sock.close()

