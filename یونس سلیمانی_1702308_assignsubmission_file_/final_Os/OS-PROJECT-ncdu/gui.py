import os
import tkinter as tk
from tkinter import filedialog
import subprocess
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from tkinter import ttk

current_dir = ""
def run_c_program(directory_path):
    process = subprocess.Popen(["./thread", directory_path], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    output, error = process.communicate()

    if error:
        return None, error.decode("utf-8")

    return output.decode("utf-8"), None


def clear_result():
    output_text.delete(1.0, tk.END)

    for widget in window.grid_slaves():
        if isinstance(widget, FigureCanvasTkAgg):
            widget.destroy()


def analyze_directory(directory_path):
    clear_result()
    output, error = run_c_program(directory_path)
    global current_dir
    current_dir = directory_path
    if error:
        result_label.config(text=f"Error: {error}")
    else:
        output_text.insert(tk.END, output)
        plot_extension_share(directory_path)

    # Clear existing items in the tree
    for child in subdirectories_tree.get_children():
        subdirectories_tree.delete(child)

    # rewrite the tree with subdirectories of the selected directory
    populate_tree(subdirectories_tree, "", directory_path)



def plot_extension_share(directory_path):
    extension_counts = {}
    total_count = 0
    try:
        with open("extension_counts.txt", "r") as file:
            for line in file:
                extension, count = line.split()
                extension_counts[extension] = int(count)
                total_count += int(count)
    except FileNotFoundError:
        return

    # Set a threshold for the share of extensions to be grouped into "Others"
    threshold = total_count/30  

    # Filter out extensions with a share below the threshold
    filtered_extensions = {ext: count for ext, count in extension_counts.items() if count >= threshold}

    # Calculate the total count of extensions below the threshold
    others_count = sum(count for count in extension_counts.values() if count < threshold)

    # Add "Others" category
    filtered_extensions["Others"] = others_count

    fig, ax = plt.subplots()
    ax.pie(filtered_extensions.values(), labels=filtered_extensions.keys(), autopct="%1.1f%%", startangle=90)
    ax.axis("equal")

    canvas = FigureCanvasTkAgg(fig, master=window)
    canvas_widget = canvas.get_tk_widget()
    canvas_widget.grid(row=2, column=0, padx=10, pady=10)


def on_tree_select(event):
    global current_dir
    item = subdirectories_tree.selection()[0]
    directory_path = current_dir + "/"+subdirectories_tree.item(item, "text") 
    analyze_directory(directory_path)

    # Clear existing items in the tree
    for child in subdirectories_tree.get_children():
        subdirectories_tree.delete(child)

    # Rewrite the tree with subdirectories of the selected directory
    populate_tree(subdirectories_tree, "", directory_path)


def close_application():
    window.destroy()

def populate_tree(tree, parent, path):
    try:
        subdirectories = [d for d in os.listdir(path) if os.path.isdir(os.path.join(path, d)) and not d.startswith('.')]
        subdirectories.insert(0,"/..")
    except OSError as e:
        print(f"Error reading directory {path}: {e}")
        return

    for subdirectory in subdirectories:
        subdir_path = os.path.join(path, subdirectory)
        tree.insert(parent, "end", text=subdirectory, values=(subdir_path,))






window = tk.Tk()
window.title("Directory Analyzer")

analyze_button = tk.Button(window, text="Select Directory", command=lambda: analyze_directory(filedialog.askdirectory()))
analyze_button.grid(row=0, column=0, pady=10)

output_text = tk.Text(window, height=15, width=50)
output_text.grid(row=1, column=0, padx=10, pady=10)

result_label = tk.Label(window, text="")
result_label.grid(row=3, column=0, pady=10)

close_button = tk.Button(window, text="Close", command=window.destroy)
close_button.grid(row=4, column=0, pady=10)

# Create a Treeview for subdirectories
subdirectories_tree = ttk.Treeview(window)
subdirectories_tree.grid(row=0, column=1, rowspan=5, padx=10, pady=10)

# Add a scrollbar to the treeview
tree_scrollbar = ttk.Scrollbar(window, orient="vertical", command=subdirectories_tree.yview)
tree_scrollbar.grid(row=0, column=2, rowspan=5, sticky="ns")
subdirectories_tree.configure(yscrollcommand=tree_scrollbar.set)

# Bind the on_tree_select function to the treeview selection event
subdirectories_tree.bind("<ButtonRelease-1>", on_tree_select)

# Populate the treeview with subdirectories starting from the current working directory
current_working_directory = os.getcwd()

window.mainloop()
