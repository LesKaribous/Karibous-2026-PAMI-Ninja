Import("env")
# Force UPLOAD_PORT to the IP set in platformio.ini, ignoring the serial port
# auto-detected by the IDE when clicking the upload button
env.Replace(UPLOAD_PORT=env.GetProjectOption("upload_port"))
