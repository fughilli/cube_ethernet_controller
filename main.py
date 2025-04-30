import asyncio
import random
from control_port import ControlPort
import time


async def generate_random_color():
    hue = (time.time() / 10) % 1
    # Convert HSV to RGB with S=1, V=0.25 (64/255)
    h = hue * 6
    c = 16  # Intensity/Value
    x = c * (1 - abs((h % 2) - 1))

    if h < 1:
        r, g, b = c, x, 0
    elif h < 2:
        r, g, b = x, c, 0
    elif h < 3:
        r, g, b = 0, c, x
    elif h < 4:
        r, g, b = 0, x, c
    elif h < 5:
        r, g, b = x, 0, c
    else:
        r, g, b = c, 0, x

    return (int(r), int(g), int(b))


async def main():
    cp = ControlPort()
    controllers = await cp.enumerate()
    for ip, ctrl in controllers.items():
        print(f"Controller at {ip} DIP={ctrl.dip}")
        await ctrl.set_lcd(0, 0, f"Hello from #{ctrl.dip}")
        await ctrl.set_backlights([1, 0, 1, 0, 1, 0])
        ctrl.register_button_callback(lambda buttons, ip=ip: print(f"{ip} buttons: {buttons}"))

    # Keep the program running and update LED colors every second
    while True:
        for ip, ctrl in controllers.items():
            color = await generate_random_color()
            print(f"Setting LED 0 on {ip} to RGB{color}")
            await ctrl.set_leds([color])
        await asyncio.sleep(1 / 60)


if __name__ == "__main__":
    asyncio.run(main())
