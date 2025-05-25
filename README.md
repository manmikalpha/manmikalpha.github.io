# Smart Air Purifier System

This project is a smart air purification system with a web-based dashboard for real-time monitoring of air quality parameters. It uses Firebase for authentication (including OAuth with Google), real-time database services, and GitHub Pages for hosting the frontend.

## Features

- Real-time display of environmental data: PM10+ (AQI), CO₂, TVOC, temperature, humidity, and pressure.
- Firebase Authentication with email/password and Google OAuth.
- Automatic, smart and manual fan control based on air quality thresholds.
- History charts for AQI, TVOC, temperature, and CO₂.
- Serverless architecture using Firebase (no backend server required).
- Hosted on GitHub Pages.

## Technologies Used

- **HTML/CSS/JavaScript** (Frontend)
- **Firebase Authentication**
- **Firebase Realtime Database**
- **GitHub Pages** (for deployment)
- **Chart.js** (for graphing history data)
- **Bootstrap 5** (for responsive UI)

## Live Demo

You can access the hosted version via GitHub Pages at: https://manmikalpha.github.io/

Follow these steps to run the project locally:

### 1. Clone the Repository

```bash
git clone https://github.com/manmikalpha/manmikalpha.github.io.git
cd manmikalpha.github.io
```
2. Set Up Firebase
   
- Go to Firebase Console
- Create a new project or use an existing one.
- Enable Email/Password and Google sign-in under Authentication > Sign-in method.
- Create a Realtime Database and set rules for development (optional for testing):

  ```bash
    {
    "rules": {
      ".read": "true",
      ".write": "true"
    }
  }
  ```
- Copy your Firebase config from the project settings and replace the placeholder inside both login.html and index.html:
  ```bash
  const firebaseConfig = {
  apiKey: "YOUR_API_KEY",
  authDomain: "YOUR_AUTH_DOMAIN",
  projectId: "YOUR_PROJECT_ID",
  databaseURL: "YOUR_DATABASE_URL",
  storageBucket: "YOUR_STORAGE_BUCKET",
  messagingSenderId: "YOUR_MESSAGING_SENDER_ID",
  appId: "YOUR_APP_ID"};
  ```
3. Open in Browser
You can open the project directly in your browser using:
```bash
# On Windows
start login.html

# On macOS
open login.html

# Or just double-click the file
```
4. Optional: Use a Local Server
For full functionality (e.g., service workers, CORS), use a local server:
✅ Option A: Using Live Server (Recommended)

- Install Visual Studio Code
- Open the project folder in VS Code.
- Install the Live Server extension from the Extensions Marketplace (by Ritwick Dey).
- Right-click on login.html and choose "Open with Live Server".
- Your browser will open at http://127.0.0.1:5500/login.html (or similar), showing the login page.
- Tip: This automatically refreshes the browser when you save changes!

✅ Option B: Using Python HTTP Server
If you have Python installed:
```bash
# For Python 3.x
python -m http.server
```
Then visit:
http://localhost:8000/login.html in your browser.
