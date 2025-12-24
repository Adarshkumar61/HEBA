<p align="center">
  <img src="demo/banner.png" width="90%">
</p>

<h1 align="center">🤖 HEBA – Assistive Robotic Arm</h1>

<p align="center">
  <b>Hospital • Elderly • Bachelors Assistant Robot</b>
</p>



# 🤖 HEBA: Robotic Arm for Hospitals, Elderly & Bachelors
========================================================

_A modular, practical robotic arm system designed for healthcare, elderly assistance, and everyday automation using embedded control and mechanical integration._

This repository contains the **HEBA robotic arm project**, including hardware connections, servo control, mission logic, diagrams, and instructions to build and control the robot.

---

## 🎯 Project Summary
--------------------------------------------------

**HEBA** stands for **Hospital, Elderly, Bachelors Assistant** — a robotic arm designed to assist in **simple pick-and-place tasks**, **delivery operations**, and **basic interaction in assistive environments**.

The goal is to provide an affordable, modular robotic arm that can:
- 🤲 Assist with lightweight object delivery
- 🩺 Support elderly or disabled users
- 🏠 Help with daily tasks for bachelors or home automation
- 🚀 Serve as an educational robotics platform

---

## 🧰 Key Features
--------------------------------------------------

### 🔹 Modular Hardware
- Predefined mechanical connections for servos, frame parts, and joints
- Full connection layout and diagrams

### 🔹 Servo Control System
- Centralized servo motor control
- Smooth motion and angle adjustments

### 🔹 Mission Logic
- Defined missions and actions for arm movement
- Can be extended for automation tasks

### 🔹 Resources Included
- Wiring diagrams
- Required item lists
- Code modules
- Project images and sketches

---

## 🗂️ Repository Structure
--------------------------------------------------

HEBA/
│
├── All_Connection/ # Hardware wiring diagrams and connection docs

├── CLAUDE/ code/ # Code and modules related to CLAUDE logic

├── Diagram/ # Mechanical and circuit diagrams

├── GPT/ Code/ # Code generated with help of GPT tools

├── Heba_Mission/ # Mission sequence and action planning

├── Instructions/ # Step-by-step setup instructions

├── Pics/ # Photos of hardware and setup

├── Required_Item_list/ # BOM (bill of materials) and parts list

├── servo/ # Servo control code and configs

└── README.md # Project documentation


---

## 🧠 Technologies Used
--------------------------------------------------

- 🧩 **Servo Motors** for mechanical motion
- 🔌 **Microcontroller / Control Logic**
- 🛠️ **Embedded C / Arduino / Customized Code**
- 📊 **Wiring and Diagrams for hardware setup**
- 📷 **Project photos & visuals**

---

## ⚙️ Setup Instructions
--------------------------------------------------

### 1️⃣ Gather Required Materials
Reference the components list:

Add the parts you need for:
- Servos
- Microcontroller board
- Power supply
- Frame parts and screws

*(A BoM table should be added in that folder if not already present.)*

---

### 2️⃣ Hardware Connections
Open:

Follow the wiring diagrams and assembly sketches.  
Build the robotic arm frame and connect servos as per the circuit.

---

### 3️⃣ Upload Code to Microcontroller
Go to:

Choose the appropriate code file and upload to the controller using your IDE (Arduino / PlatformIO).

---

### 4️⃣ Run the Control Logic
Use:

to execute predefined movement sequences.  
Ensure power is connected and servos are properly calibrated before running.

---

## 📸 Visual Demo
--------------------------------------------------



```md
### 🧱 Hardware Setup
![Hardware Setup](Pics/hardware_setup.jpg)

### 🎯 Arm in Action
![Arm Motion](Pics/arm_motion.jpg)

🧪 Expected Behavior

✔ The robotic arm powers up
✔ Servos move according to instructions
✔ Missions run as sequences of motions
✔ The system holds position and responds to commands

📈 Mission & Logic

Inside:

Heba_Mission/


You’ll find mission sequences like:

Home position

Pick position

Place position

Return to idle

These define how HEBA should behave in specific use-cases.

📚 Learning Outcomes

By building and completing HEBA you will learn:

🤖 Robotic arm kinematics and mechanics

🔧 Servo control and embedded programming

💡 Hardware wiring and circuit integration

📐 Planning of motion sequences and tasks

🚀 Future Enhancements

🤖 Add sensor feedback (limit switches, encoders)

📡 Integrate Bluetooth / Wi-Fi control

📲 Control via mobile app

📊 Add vision system for object recognition

🩺 Design extensions for healthcare applications

👨‍💻 Author

Adarsh Kumar
🎓 BCA Student | 🤖 Robotics & AI Enthusiast

🔗 GitHub: https://github.com/Adarshkumar61
