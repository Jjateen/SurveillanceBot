import numpy as np
import imutils
import cv2
import requests

prototxt = "MobileNetSSD_deploy.prototxt.txt"
model = "MobileNetSSD_deploy.caffemodel"
confThresh = 0.2

CLASSES = ["background", "aeroplane", "bicycle", "bird", "boat",
	"bottle", "bus", "car", "cat", "chair", "cow", "diningtable",
	"dog", "horse", "motorbike", "person", "pottedplant", "sheep",
	"sofa", "train", "tvmonitor"]

COLORS = np.random.uniform(0, 255, size=(len(CLASSES), 3))

print("Loading model...")
net = cv2.dnn.readNetFromCaffe(prototxt, model)
print("Model Loaded")

# Replace the URL with the URL of your video stream
url = "http://192.168.101.65:81/stream"

while True:
    response = requests.get(url, stream=True)
    if response.status_code == 200:
        bytes = bytes()
        for chunk in response.iter_content(chunk_size=1024):
            bytes += chunk
            a = bytes.find(b'\xff\xd8')
            b = bytes.find(b'\xff\xd9')
            if a != -1 and b != -1:
                jpg = bytes[a:b + 2]
                bytes = bytes[b + 2:]
                if len(jpg) > 0:
                    nparr = np.frombuffer(jpg, dtype=np.uint8)
                    frame = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
                    frame = cv2.resize(frame, (960, 520))
                    # frame = imutils.resize(frame, width=500)
                    (h, w) = frame.shape[:2]
                    imResize = cv2.resize(frame, (300, 300))
                    blob = cv2.dnn.blobFromImage(imResize, 0.007843, (300, 300), 127.5)

                    net.setInput(blob)
                    detections = net.forward()

                    detShape = detections.shape[2]
                    for i in np.arange(0, detShape):
                        confidence = detections[0, 0, i, 2]
                        if confidence > confThresh:
                            idx = int(detections[0, 0, i, 1])
                            box = detections[0, 0, i, 3:7] * np.array([w, h, w, h])
                            (startX, startY, endX, endY) = box.astype("int")

                            label = "{}: {:.2f}%".format(CLASSES[idx], confidence * 100)
                            cv2.rectangle(frame, (startX, startY), (endX, endY), COLORS[idx], 2)
                            if startY - 15 > 15:
                                y = startY - 15
                            else:
                                y = startY + 15
                            cv2.putText(frame, label, (startX, y), cv2.FONT_HERSHEY_SIMPLEX, 0.5, COLORS[idx], 2)

                    cv2.imshow("Frame", frame)
                    key = cv2.waitKey(1)
                    if key == 27:
                        break

cv2.destroyAllWindows()
