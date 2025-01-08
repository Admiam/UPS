import tkinter as tk


class WaitingScreen:
    def __init__(self, root):
        """
        Initialize the waiting screen.

        Args:
            root (tk.Tk): The Tkinter root or Toplevel window for this screen.
        """
        self.root = root
        self.root.title("Waiting for Group Assignment")

        # Waiting message
        tk.Label(root, text="Waiting for other players...", font=("Arial", 16)).pack(pady=20)

        # Cancel button to close the waiting screen and disconnect
        cancel_button = tk.Button(root, text="Cancel", command=self.cancel_waiting)
        cancel_button.pack(pady=10)

    def cancel_waiting(self):
        """
        Handle waiting screen cancellation. This should delegate cleanup to the ServerListener.
        """
        # ServerListener should handle socket cleanup and screen transition
        print("Cancel waiting - delegate to ServerListener.")
