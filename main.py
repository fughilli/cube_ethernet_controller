import asyncio
import random
import textwrap  # Import textwrap module
from control_port import ControlPort

# --- Joke Setup ---
JOKES = {
    0: "Why don't scientists trust atoms? Because they make up everything!",
    1: "What do you call a lazy kangaroo? Pouch potato!",
    2: "Why did the scarecrow win an award? Because he was outstanding in his field!",
    3: "What did the left eye say to the right eye? Between you and me, something smells!",
    4: "Why don't skeletons fight each other? They don't have the guts.",
    5: "What do you call fake spaghetti? An impasta!",
}
DEFAULT_JOKE = "Press a button for a joke!"
LAST_BUTTON_PRESS = {}


async def generate_random_color():
    hue = random.random()  # Random hue between 0 and 1
    # Convert HSV to RGB with S=1, V=0.25 (64/255)
    h = hue * 6
    c = 64  # Intensity/Value
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


# --- Button Callback ---
def handle_button_press(buttons, ip, ctrl):
    global LAST_BUTTON_PRESS
    print(f"{ip} buttons: {buttons}")

    pressed_button = -1
    # Find the first button that is pressed (state == 1)
    try:
        pressed_button = buttons.index(1)
    except ValueError:
        # No button pressed
        pass

    last_pressed = LAST_BUTTON_PRESS.get(ip, -1)

    # Only trigger update on state change
    if pressed_button != last_pressed:
        # --- Clear LCD First using the dedicated method ---
        # Use asyncio.create_task to run clear without blocking the callback
        # if we don't need to strictly wait for it before sending the joke.
        # If strict order is needed, use 'await ctrl.clear_lcd()'
        asyncio.create_task(ctrl.clear_lcd())
        # Optional small delay if needed for visual separation
        # await asyncio.sleep(0.05)

        # --- Now display the joke ---
        joke = DEFAULT_JOKE
        if pressed_button != -1:  # A button is pressed
            joke = JOKES.get(pressed_button, DEFAULT_JOKE)

        print(f"Displaying on {ip}: {joke[:20]}...")

        # Wrap text for 20-char width LCD
        wrapped_lines = textwrap.wrap(joke, width=20)

        # Send up to 4 lines to the LCD
        joke_tasks = []
        for i in range(4):
            line_text = " " * 20  # Default to clear line
            if i < len(wrapped_lines):
                line_text = wrapped_lines[i].ljust(20)  # Get wrapped line and pad
            joke_tasks.append(ctrl.set_lcd(0, i, line_text))

        # Create tasks to send LCD commands asynchronously
        asyncio.gather(*[asyncio.create_task(task) for task in joke_tasks])

        LAST_BUTTON_PRESS[ip] = pressed_button


async def main():
    cp = ControlPort()
    controllers = await cp.enumerate()
    for ip, ctrl in controllers.items():
        print(f"Controller at {ip} DIP={ctrl.dip}")
        await ctrl.set_lcd(0, 0, f"Hello from #{ctrl.dip}")
        await ctrl.set_lcd(0, 1, DEFAULT_JOKE[:20])  # Show default message initially
        await ctrl.set_backlights([0, 0, 0, 0, 0, 0])  # Start with backlights off
        # Register the callback, passing the controller object
        ctrl.register_button_callback(
            lambda buttons, ip=ip, ctrl=ctrl: handle_button_press(buttons, ip, ctrl)
        )
        LAST_BUTTON_PRESS[ip] = -1  # Initialize last state

    # Keep the program running and update LED colors every second
    led_index = 0  # Restore LED cycling index
    while True:
        tasks = []
        for ip, ctrl in controllers.items():
            color = await generate_random_color()
            # Cycle through all LEDs
            led_states = [(0, 0, 0)] * 6
            led_states[led_index % 6] = color  # Set the current LED in the cycle
            tasks.append(ctrl.set_leds(led_states))

            # Only control backlight 4 due to hardware limitations
            backlight_states = [0] * 6
            backlight_states[4] = 1  # Always turn on only backlight 4
            tasks.append(ctrl.set_backlights(backlight_states))

        await asyncio.gather(*tasks)  # Run LED/backlight updates concurrently
        led_index += 1  # Increment LED index for the next cycle
        await asyncio.sleep(0.2)  # Keep update cycle


if __name__ == "__main__":
    asyncio.run(main())
