import customtkinter as ctk

# Set the appearance mode
ctk.set_appearance_mode("dark")
ctk.set_default_color_theme("blue")

# Create the main window
app = ctk.CTk()
app.geometry("400x300")
app.title("PLC Ladder Editor - Hello")

# Add a label
label = ctk.CTkLabel(app, text="Hello, PLC Ladder Editor!", font=("Arial", 18))
label.pack(pady=40)

# Add a button
def on_click():
    label.configure(text="Connected to STM32! (just kidding)")

button = ctk.CTkButton(app, text="Test Button", command=on_click)
button.pack(pady=20)

# Run the app
app.mainloop()