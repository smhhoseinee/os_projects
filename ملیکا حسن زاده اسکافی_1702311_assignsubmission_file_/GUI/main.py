import tkinter as tk
from tkinter import filedialog
import subprocess

selected_directory = ""  # Global variable to store the selected directory


def browse_directory():
    global selected_directory
    directory_path = filedialog.askdirectory()
    if directory_path:
        selected_directory = directory_path
        entry_var.set(selected_directory)
        status_label.config(text="Directory saved: " + selected_directory)


def run_c_program():
    global selected_directory
    if selected_directory:
        # Run the C program as a subprocess
        command = ["/home/fatima/CLionProjects/semiNCDU_GIT/src/main.out", selected_directory]
        process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        output, error = process.communicate()

        # Display the output in the Tkinter GUI
        result_window = tk.Toplevel(root)
        result_window.title("Monitoring")

        # Create a Text widget to display the output
        result_text = tk.Text(result_window, wrap=tk.WORD, width=80, height=20)
        result_text.pack(padx=10, pady=10)

        # Insert the captured output into the Text widget
        result_text.insert(tk.END, output.decode("utf-8"))

        # Display error, if any
        if error:
            # Insert error message in red
            result_text.insert(tk.END, "\nError:\n" + error.decode("utf-8"), "error")

        # Display the C program output in the terminal
        print(output.decode("utf-8"))

    else:
        status_label.config(text="Please select a directory first.")


# Create the main window
root = tk.Tk()
root.title("Directory Selector")

# Configure the window size and background color
root.geometry("500x250")
root.configure(bg="#F0F0F0")

# Create a label and an entry widget
label = tk.Label(root, text="Selected Directory:", bg="#F0F0F0", font=("Helvetica", 14))
label.pack(pady=10)

entry_var = tk.StringVar()
entry = tk.Entry(root, textvariable=entry_var, width=40, state="readonly", font=("Helvetica", 12))
entry.pack(pady=10)

# Create a button to open a directory selection dialog
browse_button = tk.Button(root, text="Browse", command=browse_directory, font=("Helvetica", 12))
browse_button.pack(pady=10)

# Create a label to display the status
status_label = tk.Label(root, text="", bg="#F0F0F0", font=("Helvetica", 12, "italic"), fg="#0066cc")
status_label.pack(pady=10)

# Create a button to run the C program
run_c_button = tk.Button(root, text="Run C Program", command=run_c_program, font=("Helvetica", 12), bg="#FF5733", fg="white")
run_c_button.pack(pady=10)

# Run the main loop
root.mainloop()