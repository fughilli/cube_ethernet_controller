import asyncio
from control_port import ControlPort

async def main():
    cp = ControlPort()
    controllers = await cp.enumerate()
    for ip, ctrl in controllers.items():
        print(f"Controller at {ip} DIP={ctrl.dip}")
        ctrl.set_lcd(0, 0, f"Hello from #{ctrl.dip}")
        ctrl.set_backlights([1, 0, 1, 0, 1, 0])
        ctrl.register_button_callback(lambda buttons, ip=ip: print(f"{ip} buttons: {buttons}"))
    try:
        while True:
            await asyncio.sleep(1)
    except asyncio.CancelledError:
        print("Shutting down...")

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("Exiting on user request.") 