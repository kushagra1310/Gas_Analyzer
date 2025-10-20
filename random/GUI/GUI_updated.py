"""
Enhanced Gas Analyzer GUI with a static gradient background, canvas-drawn text for
clear visibility, and a fixed graph legend.
"""
import tkinter as tk
import random
from datetime import datetime, timedelta
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import matplotlib
matplotlib.use('Agg')
import io
from PIL import Image, ImageTk

import socket
import threading
import queue
import time

ESP32_IP = "10.69.186.93"
ESP32_PORT = 8080


# --- Thresholds ---
THRESHOLDS = {'MQ-3': 0.5, "MQ-135":0.5, 'MQ-136': 0.5, "MQ-137": 0.5}

# --- Sensor Data Simulation ---
SENSORS = {
    "MQ-3": {"status": "On", "unit": "Rs/R0", "start_time":None},
    "MQ-135": {"status": "Off", "unit": "Rs/R0", "start_time": None},
    "MQ-136": {"status": "On", "unit": "Rs/R0", "start_time": None},
    "MQ-137": {"status": "On", "unit": "Rs/R0", "start_time": None}
}
sensor_historical_data = {name: [0 for _ in range(50)] for name in SENSORS}

# --- Utility Functions ---
def rgb_to_hex(rgb):
    """Convert (r,g,b) tuple to hex color string."""
    return f"#{int(rgb[0]):02x}{int(rgb[1]):02x}{int(rgb[2]):02x}"

def gradient_color(start, end, steps):
    """Generate a list of colors forming a gradient."""
    return [
        rgb_to_hex((
            start[0] + (end[0] - start[0]) * i / (steps - 1),
            start[1] + (end[1] - start[1]) * i / (steps - 1),
            start[2] + (end[2] - start[2]) * i / (steps - 1),
        ))
        for i in range(steps)
    ]

def create_sparkline(data, width=80, height=28):
    """
    Create a sparkline image from a list of data points.
    Returns a Tkinter PhotoImage.
    """
    fig = Figure(figsize=(width/100, height/100), dpi=100)
    ax = fig.add_subplot(111)
    ax.plot(data, color='#00eaff', linewidth=2)
    ax.axis('off')
    fig.subplots_adjust(left=0, right=1, top=1, bottom=0)
    buf = io.BytesIO()
    fig.savefig(buf, format='png', transparent=True, bbox_inches='tight', pad_inches=0)
    buf.seek(0)
    img = Image.open(buf)
    img = img.resize((width, height), Image.LANCZOS)
    photo = ImageTk.PhotoImage(img)
    buf.close()
    return photo

# --- Static Gradient Canvas ---
class GradientCanvas(tk.Canvas):
    """Canvas that draws a static vertical gradient background and blinking stars."""
    def __init__(self, master, color1, color2, **kwargs):
        super().__init__(master, **kwargs)
        self.color1 = color1
        self.color2 = color2
        self.stars = []
        self.star_objs = []
        self.after_id = None
        self.bind("<Configure>", self.draw_gradient)
        self.init_stars()
        self.animate_stars()

    def draw_gradient(self, event=None):
        """Draw vertical gradient rectangles filling the canvas."""
        self.delete("gradient")
        width = self.winfo_width()
        height = self.winfo_height()
        if not width or not height:
            return
        limit = 100
        gradient_colors = gradient_color(self.color1, self.color2, limit)
        for i, color in enumerate(gradient_colors):
            y1 = i * height / limit
            y2 = (i + 1) * height / limit
            self.create_rectangle(0, y1, width, y2, fill=color, outline="", tags="gradient")
        self.draw_stars()

    def init_stars(self, num_stars=40):
        """Randomly place stars in the background."""
        self.stars = []
        for _ in range(num_stars):
            x = random.randint(10, 840)
            y = random.randint(10, 590)
            r = random.randint(1, 3)
            blink_speed = random.randint(10, 30)
            self.stars.append({"x": x, "y": y, "r": r, "on": True, "blink": blink_speed, "count": random.randint(0, blink_speed)})

    def draw_stars(self):
        """Draw all stars, blinking some of them."""
        self.delete("star")
        for star in self.stars:
            color = "#fff" if star["on"] else "#555"
            self.create_oval(
                star["x"] - star["r"], star["y"] - star["r"],
                star["x"] + star["r"], star["y"] + star["r"],
                fill=color, outline="", tags="star"
            )

    def animate_stars(self):
        """Blink stars with random delay."""
        for star in self.stars:
            star["count"] += 1
            if star["count"] >= star["blink"]:
                star["on"] = not star["on"]
                star["count"] = 0
        self.draw_stars()
        self.after_id = self.after(200, self.animate_stars)

# --- Sensor Card Widget ---
class SensorCard(tk.Frame):
    """
    Card-style frame that draws sensor info directly onto a canvas for a clean look.
    Adds: threshold color alerts, sparkline graphs.
    """
    def __init__(self, master, sensor_name, on_click, height=60, **kwargs):
        super().__init__(master, height=height, **kwargs)
        self.sensor_name = sensor_name
        self.on_click = on_click
        self.ripple_id = None
        self.hovered = False

        # Canvas for background and text
        self.card_canvas = tk.Canvas(self, bd=0, highlightthickness=0, bg="#2c3e50", height=height)
        self.card_canvas.pack(fill="both", expand=True)
        self.card_canvas.config(height=height)
        self.config(height=height)
        self.pack_propagate(False)

        display_name = f"{sensor_name}"

        # --- Adjusted positions for perfect column alignment ---
        self.name_text_id = self.card_canvas.create_text(
            0, height//2, anchor="w", font=("Orbitron", 18, "bold"), fill="#00eaff", text=display_name
        )
        # Status column: align left, not center, for better header match
        self.status_text_id = self.card_canvas.create_text(
            170, height//2, anchor="w", font=("Orbitron", 16, "bold"), fill="#bdc3c7", text="N/A"
        )
        self.sparkline_label = tk.Label(self.card_canvas, bg="#2c3e50", bd=0)
        self.sparkline_window = self.card_canvas.create_window(320, height//2, window=self.sparkline_label, anchor="w")
        self.data_text_id = self.card_canvas.create_text(
            470, height//2, anchor="w", font=("Orbitron", 18, "bold"), fill="#ffffff", text="---"
        )

        self.bind_events()
        self.card_canvas.bind("<Configure>", self.align_text)

    def align_text(self, event=None):
        w = self.winfo_width()
        h = self.winfo_height()
        # These fractions match the header grid columns visually
        self.card_canvas.coords(self.name_text_id, int(w * 0.05), h // 2)
        self.card_canvas.coords(self.status_text_id, int(w * 0.28), h // 2)
        self.card_canvas.coords(self.sparkline_window, int(w * 0.48), h // 2)
        self.card_canvas.coords(self.data_text_id, int(w * 0.73), h // 2)

    def bind_events(self):
        """Bind mouse events for a seamless experience."""
        self.card_canvas.bind("<Enter>", self.on_hover)
        self.card_canvas.bind("<Leave>", self.on_leave)
        self.card_canvas.bind("<Button-1>", self.on_click_event)

    def on_hover(self, event=None):
        self.hovered = True
        self.card_canvas.config(cursor="hand2")
        self.draw_border()

    def on_leave(self, event=None):
        self.hovered = False
        self.card_canvas.config(cursor="arrow")
        self.draw_border()

    def on_click_event(self, event=None):
        x = event.x if event else self.winfo_width() // 2
        y = event.y if event else self.winfo_height() // 2
        self.show_ripple(x, y)
        self.after(180, lambda: self.on_click(self.sensor_name))

    def show_ripple(self, x, y):
        if self.ripple_id: self.card_canvas.delete(self.ripple_id)
        r = 0
        def animate():
            nonlocal r
            if r > self.winfo_width():
                self.card_canvas.delete("ripple")
                return
            self.ripple_id = self.card_canvas.create_oval(
                x - r, y - r, x + r, y + r, outline="#5dade2", width=2, tags="ripple"
            )
            r += 12
            self.after(18, animate)
        animate()

    def draw_border(self):
        """Draw a border on the canvas to indicate hover/status."""
        self.card_canvas.delete("border")
        status_color = self.card_canvas.itemcget(self.status_text_id, "fill")
        self.card_canvas.create_rectangle(
            0, 0, self.winfo_width(), self.winfo_height(),
            outline=status_color, width=2 if self.hovered else 1, tags="border"
        )
        
    def update_text(self, status_text, status_color, data_text, data_color, sparkline_img):
        """Update the text and colors on the canvas, and sparkline image."""
        self.card_canvas.itemconfig(self.status_text_id, text=status_text, fill=status_color)
        self.card_canvas.itemconfig(self.data_text_id, text=data_text, fill=data_color)
        self.sparkline_label.configure(image=sparkline_img)
        self.sparkline_label.image = sparkline_img
        self.draw_border()

# --- NEW Prediction Card Widget ---
class PredictionCard(tk.Frame):
    """
    A dedicated card-style frame to display the ML model's prediction.
    """
    def __init__(self, master, height=60, **kwargs):
        super().__init__(master, height=height, **kwargs)
        
        # Canvas for background and text
        self.card_canvas = tk.Canvas(self, bd=0, highlightthickness=0, bg="#2c504e", height=height)
        self.card_canvas.pack(fill="both", expand=True)
        self.config(height=height)
        self.pack_propagate(False)

        # Title and prediction text drawn directly on the canvas
        self.title_text_id = self.card_canvas.create_text(
            0, height//2, anchor="w", font=("Orbitron", 18, "bold"), 
            fill="#00eaff", text="PREDICTION"
        )
        self.prediction_text_id = self.card_canvas.create_text(
            0, height//2, anchor="e", font=("Orbitron", 22, "bold"), 
            fill="#00ff00", text="--"
        )

        self.card_canvas.bind("<Configure>", self.align_text)
        self.draw_border()

    def align_text(self, event=None):
        w = self.winfo_width()
        h = self.winfo_height()
        # Align title to the left and prediction to the right
        self.card_canvas.coords(self.title_text_id, int(w * 0.05), h // 2)
        self.card_canvas.coords(self.prediction_text_id, int(w * 0.95), h // 2)

    def draw_border(self):
        """Draw a border on the canvas."""
        self.card_canvas.delete("border")
        self.card_canvas.create_rectangle(
            0, 0, self.winfo_width(), self.winfo_height(),
            outline="#ff9800", width=2, tags="border"
        )
        
    def update_prediction(self, prediction_text):
        """Update the prediction text on the canvas."""
        self.card_canvas.itemconfig(self.prediction_text_id, text=prediction_text)


# NEW PART
def network_client_thread(data_queue, status_queue):
    """
    This function runs in a separate thread.
    It continuously tries to connect to the ESP32 and fetch data.
    """
    while True:
        try:
            status_queue.put(f"Connecting to {ESP32_IP}...")
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.settimeout(5)
                s.connect((ESP32_IP, ESP32_PORT))
                status_queue.put(f"Connected to {ESP32_IP}")
                
                buffer = ""
                while True:
                    # Receive data in chunks
                    data = s.recv(128).decode('utf-8', errors='ignore')
                    if not data:
                        break # Connection closed
                    
                    buffer += data
                    # Process lines one by one
                    while '\n' in buffer:
                        line, buffer = buffer.split('\n', 1)
                        if line.strip():
                            data_queue.put(line.strip())

        except socket.timeout:
            status_queue.put(f"Connection to {ESP32_IP} timed out. Retrying...")
        except ConnectionRefusedError:
             status_queue.put(f"Connection refused by {ESP32_IP}. Retrying...")
        except Exception as e:
            status_queue.put(f"Network Error: {e}. Retrying...")
        
        # Wait before retrying
        random_wait = 2 + random.random() * 3
        time.sleep(random_wait)

# --- Main Application Class ---
class GasAnalyzerApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Gas Analyzer")
        self.root.geometry("850x600")
        self.root.minsize(700, 500)

        self.bg_canvas = GradientCanvas(root, color1=(44, 62, 80), color2=(24, 26, 42))
        self.bg_canvas.place(relwidth=1, relheight=1)

        self.main_frame = tk.Frame(root, bg="#232946")
        self.main_frame.place(relx=0.5, rely=0.5, anchor="center", relwidth=0.9, relheight=0.9)

        self.create_header()
        self.sensor_cards = {}
        self.prediction_card = None
        self.create_sensor_table()

        self.status_var = tk.StringVar(value="Initializing...")
        self.status_bar = tk.Label(
            self.main_frame, textvariable=self.status_var,
            font=("Orbitron", 12), fg="#00eaff", bg="#232946"
        )
        self.status_bar.pack(side="bottom", fill="x", pady=5)

        # --- MODIFICATION START: Create queue and start network thread ---
        self.data_queue = queue.Queue()
        self.status_queue = queue.Queue()

        self.network_thread = threading.Thread(
            target=network_client_thread,
            args=(self.data_queue, self.status_queue),
            daemon=True  # Allows main program to exit even if thread is running
        )
        self.network_thread.start()

        self.update_gui()
        # --- MODIFICATION END ---

    def create_header(self):
        header_container = tk.Frame(self.main_frame, bg="#232946")
        header_container.pack(fill="x", pady=10, padx=10)
        
        tk.Label(
            header_container, text="BANANA CHIPS", font=("Orbitron", 22, "bold"), 
            fg="#ff9800", bg="#232946"
        ).pack()
        tk.Label(
            header_container, text="GAS ANALYZER", font=("Orbitron", 36, "bold"), 
            fg="#00eaff", bg="#232946"
        ).pack(pady=(0, 10))

        tk.Frame(header_container, bg="#00eaff", height=3).pack(fill="x", pady=5)

        # --- Organize columns in a single row using grid ---
        header_frame = tk.Frame(header_container, bg="#232946")
        header_frame.pack(fill="x", expand=True, pady=(5,0))
        columns = ["SENSOR", "STATUS", "TREND", "LIVE DATA"]
        for i, col in enumerate(columns):
            tk.Label(
                header_frame, text=col, font=("Orbitron", 16, "bold"),
                fg="#00eaff", bg="#232946"
            ).grid(row=0, column=i, sticky="w", padx=(int(0.05 * 850) if i == 0 else 0, 0))
            header_frame.grid_columnconfigure(i, weight=1)

    def create_sensor_table(self):
        self.cards_frame = tk.Frame(self.main_frame, bg="#232946")
        self.cards_frame.pack(fill="both", expand=True, padx=10, pady=10)

        for idx, name in enumerate(SENSORS):
            card = SensorCard(
                self.cards_frame, name, on_click=self.open_detail_window, height=60
            )
            card.grid(row=idx, column=0, sticky="ew", pady=5)
            card.card_canvas.config(height=60)
            card.config(height=60)
            card.pack_propagate(False)
            self.cards_frame.grid_rowconfigure(idx, weight=0)
            self.sensor_cards[name] = card

        # Create and place the dedicated prediction card below all sensors
        self.prediction_card = PredictionCard(self.cards_frame, height=60)
        self.prediction_card.grid(row=len(SENSORS), column=0, sticky="ew", pady=(15, 5)) # Add extra top padding

        self.cards_frame.grid_columnconfigure(0, weight=1)

    # ---Renamed and rewritten update loop ---
    def update_gui(self):
        """
        Processes messages from the network thread's queue to update the GUI.
        """
        # Process all pending messages in the data queue
        while not self.data_queue.empty():
            try:
                # Expected format: "SENSOR_NAME,VALUE" or "prediction,GAS_NAME"
                message = self.data_queue.get_nowait()
                
                parts = message.split(',')
                if len(parts) != 2:
                    print(f"Malformed data received: {message}")
                    continue

                name, value_str = parts[0].strip(), parts[1].strip()

                # --- FIX IS HERE: Check for prediction FIRST ---
                if name == "Prediction":
                    if self.prediction_card:
                        self.prediction_card.update_prediction(value_str.upper())

                elif name in self.sensor_cards:
                    # <-- FIX: Convert to float only if it's a sensor message
                    new_reading = float(value_str) 
                    
                    # Update sensor's historical data
                    sensor_historical_data[name].pop(0)
                    sensor_historical_data[name].append(new_reading)

                    # Update the visual card
                    card = self.sensor_cards[name]
                    details = SENSORS[name]
                    
                    threshold = THRESHOLDS.get(name)
                    data_color = '#FF9900' if (threshold and new_reading < threshold) else '#ffffff'
                    
                    spark_img = create_sparkline(sensor_historical_data[name], width=80, height=28)
                    
                    card.update_text(
                        status_text="● ON", status_color="#00ff99",
                        data_text=f"{new_reading:.2f} {details['unit']}", data_color=data_color,
                        sparkline_img=spark_img
                    )
            except queue.Empty:
                pass # No more messages
            except (ValueError, IndexError) as e:
                # This will now correctly catch errors only for sensor messages
                print(f"Error processing data '{message}': {e}")
                
        # Update the status bar from the status queue
        while not self.status_queue.empty():
            try:
                status_message = self.status_queue.get_nowait()
                self.status_var.set(f"System Status: {status_message} | Last Update: {datetime.now().strftime('%H:%M:%S')}")
            except queue.Empty:
                pass

        # Schedule the next check
        self.root.after(100, self.update_gui)
    
    def open_detail_window(self, sensor_name):
        detail_window = tk.Toplevel(self.root)
        detail_window.title(f"{sensor_name} Details")
        # Increased window size for bigger graph
        detail_window.geometry("850x600")
        detail_window.transient(self.root)
        detail_window.grab_set()

        bg_canvas = GradientCanvas(detail_window, color1=(24, 26, 42), color2=(44, 62, 80))
        bg_canvas.place(relwidth=1, relheight=1)

        tk.Label(
            detail_window, text=f"{sensor_name} DETAILS", font=("Orbitron", 28, "bold"),
            fg="#00eaff", bg="#181a2a"
        ).pack(pady=28)
        
        top_info_frame = tk.Frame(detail_window, bg="#181a2a")
        top_info_frame.pack(fill="x", padx=30, pady=10)

        current_reading_var = tk.StringVar()
        current_card = tk.Frame(top_info_frame, bg="#232946")
        current_card.pack(side="left", expand=True, padx=10, fill="y")
        current_reading_label = tk.Label(
            current_card, textvariable=current_reading_var, font=("Orbitron", 26, "bold"),
            fg="#00ff99", bg="#232946"
        )
        current_reading_label.pack(pady=(10,0), expand=True)
        tk.Label(current_card, text="CURRENT READING", font=("Orbitron", 14), fg="#bdc3c7", bg="#232946").pack(pady=(0,10), expand=True)

        since_var = tk.StringVar()
        since_card = tk.Frame(top_info_frame, bg="#232946")
        since_card.pack(side="right", expand=True, padx=10, fill="y")
        tk.Label(since_card, textvariable=since_var, font=("Orbitron", 26, "bold"), fg="#00eaff", bg="#232946").pack(pady=(10,0), expand=True)
        tk.Label(since_card, text="ACTIVE SINCE", font=("Orbitron", 14), fg="#bdc3c7", bg="#232946").pack(pady=(0,10), expand=True)

        # Increased graph frame and figure size
        graph_frame = tk.Frame(detail_window, bg="#232946")
        graph_frame.pack(fill="both", expand=True, padx=30, pady=30)
        
        fig = Figure(figsize=(8, 3.8), dpi=100, facecolor="#232946")
        plot = fig.add_subplot(1, 1, 1)
        canvas = FigureCanvasTkAgg(fig, master=graph_frame)
        canvas.get_tk_widget().pack(fill="both", expand=True)

        def update_detail():
            if not detail_window.winfo_exists(): return
            
            card = self.sensor_cards[sensor_name]
            current_text = card.card_canvas.itemcget(card.data_text_id, 'text')
            current_reading_var.set(current_text)
            
            sensor_details = SENSORS[sensor_name]
            threshold = THRESHOLDS.get(sensor_name)
            try:
                latest_value = float(current_text.split()[0])
            except Exception:
                latest_value = None

            if threshold and latest_value and latest_value < threshold:
                current_reading_label.config(fg="#FF9900")
            else:
                current_reading_label.config(fg="#00ff99")
            
            if sensor_details["status"] == "On" and sensor_details["start_time"]:
                delta = datetime.now() - sensor_details["start_time"]
                hours, rem = divmod(delta.total_seconds(), 3600)
                mins = (rem % 3600) // 60
                since_var.set(f"{int(hours)}h {int(mins)}m")
            else:
                since_var.set("OFFLINE")
            
            plot.clear()
            if threshold:
                plot.axhline(threshold, color="#FF9900", linestyle="--", linewidth=2, label="Threshold")
            plot.plot(
                sensor_historical_data[sensor_name], color="#00eaff", linewidth=2,
                marker='o', markersize=4, markerfacecolor='#232946', markeredgecolor='#00eaff',
                label=f"{sensor_name} Reading (Rs/R0)"
            )
            plot.set_title(f"{sensor_name} Reading (Rs/R0)", color="#00eaff", fontsize=16, pad=24)
            plot.set_facecolor("#232946")
            plot.tick_params(axis='x', colors='#bdc3c7')
            plot.tick_params(axis='y', colors='#bdc3c7')
            for spine in ['bottom', 'left']: plot.spines[spine].set_color('#00eaff')
            for spine in ['top', 'right']: plot.spines[spine].set_color('#232946')
            plot.grid(True, linestyle='--', linewidth=0.5, color='#5dade2')
            legend = plot.legend(
                facecolor="#232946", edgecolor="#00eaff", fontsize=12, labelcolor='#ffffff',
                loc='lower center', bbox_to_anchor=(0.5, 1.13), borderaxespad=3, ncol=2
            )
            fig.tight_layout(pad=3.0)
            canvas.draw()
            detail_window.after(2000, update_detail)
        update_detail()

        close_btn = tk.Label(detail_window, text="✕", font=("Orbitron", 20, "bold"), fg="#bdc3c7", bg="#181a2a", cursor="hand2")
        close_btn.place(relx=1.0, rely=0.0, anchor="ne", x=-8, y=8)
        close_btn.bind("<Button-1>", lambda e: detail_window.destroy())

if __name__ == "__main__":
    root = tk.Tk()
    app = GasAnalyzerApp(root)
    root.mainloop()
