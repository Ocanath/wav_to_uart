from wav_serial import SerialPlayer



def main():
    """Main control thread"""
    # Configure your serial port here
    # port = 'COM3'  # Change to your port

    player = SerialPlayer()
    player.start()

    print("Serial Audio Player")
    print("Commands:")
    print("  play <filename>  - Play a WAV file")
    print("  stop             - Stop current playback")
    print("  quit             - Exit program")
    print()

    try:
        while True:
            user_input = input("> ").strip()

            if not user_input:
                continue

            parts = user_input.split()
            command = parts[0].lower()

            if command == 'play':
                if len(parts) < 2:
                    print("Usage: play <filename>")
                    continue
                filename = parts[1]
                player.play(filename)

            elif command == 'stop':
                player.abort()
                print("Stopping playback...")

            elif command == 'quit' or command == 'exit':
                print("Exiting...")
                break

            else:
                print(f"Unknown command: {command}")

    except KeyboardInterrupt:
        print("\nInterrupted")
    finally:
        player.stop()
        print("Goodbye!")


if __name__ == "__main__":
    main()
