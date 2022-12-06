from flask import Flask
import asyncio
import cv2
import face_recognition
import numpy as np
from threading import *
import threading
import time
# from pywizlight import wizlight, PilotBuilder, discovery

app = Flask(__name__)
lock = threading.Lock()
authorized = 0

#This should be the IP of the ESP running 'phread_face_authorize' given
#on initialization in the Serial Monitor
camera_IP = "YOUR IP"


#this will turn on Wiz lights using API
# async def turnOnLights():
#     lightOver = wizlight("10.0.0.199") #my overhead
#     lightLamp = wizlight("10.0.0.220") #my lamp
#
#     await lightOver.turn_on(PilotBuilder(brightness = 50))
#     await lightOver.turn_on(PilotBuilder(rgb = (0, 100, 50)))
#
#     await lightLamp.turn_on(PilotBuilder(brightness = 50))
#     await lightLamp.turn_on(PilotBuilder(rgb = (0, 100, 50)))
def turnOnLights():
    pass


@app.route('/')
def index():
    return "Hello, World"


#using the /auth tag, we can directly authorize entrance. Eventually, want
#to take photo/video data and only authorize if it recognized user face
@app.route('/auth/<open>')
def authorize(open):
    auth(open)
    return open


def auth(op):

    global authorized
    if(op == "1" or op == "2"):
        authorized = 1
    else:
        authorized = 0

    if(op == "2"):
        print("AUTHED")
        turnOnLights()
    #     #if this is too slow, do it elsewhere
    #     # loop = asyncio.get_event_loop()
    #     # loop.run_until_complete(turnOnLights())
        # turnOnLights()
    #     pass

    return op


@app.route('/go')
def go():
    global authorized
    toReturn = "0"

    if(authorized == 1):
        toReturn = "1"
        authorized = 0

    return toReturn


def faceProcessing():
    global authorized

    img2 = cv2.imread("ElonMusk.jpg")
    rgb_img2 = cv2.cvtColor(img2, cv2.COLOR_BGR2RGB)
    img2_encoding = face_recognition.face_encodings(rgb_img2)[0]

    #encode faces for recognition
    # img = cv2.imread("YOUR_PATH.jpg")
    # rgb_img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    # img_encoding = face_recognition.face_encodings(rgb_img)[0]

    #create arrays for encodings
    known_face_encodings = [
        img_encoding,
        # img2_encoding
    ]
    #add names to THIS list
    known_face_names = [
        "ElonMusk",
        # "Your name here"
    ]

    #init face rec vars
    face_locations = []
    face_encodings = []
    face_names = []
    process_this_frame = True

    #below IP needs to be IP of video capture ESP given when starting the
    #program 'phread_face_authorize' in Serial Monitor
    cam_URL = "http://" + camera_IP + ":81/stream"
    capture = cv2.VideoCapture(cam_URL)

    while(True):
        ret, frm = capture.read()

        #Detect faces
        frame = cv2.flip(frm, 0)

        if process_this_frame:
            #convert to 1/4 size for faster recognition
            small_frame = cv2.resize(frame, (0, 0), fx=0.25, fy=0.25)

            #convert from BGR to RGB
            rgb_small_frame = small_frame[:, :, ::-1]

            #find all faces and encodings in current frame
            face_locations = face_recognition.face_locations(rgb_small_frame)
            face_encodings = face_recognition.face_encodings(rgb_small_frame, face_locations)

            face_names = []
            for face_encoding in face_encodings:
                #see if the face is a match for known faces
                matches = face_recognition.compare_faces(known_face_encodings, face_encoding)
                name = "Unknown"

                face_distances = face_recognition.face_distance(known_face_encodings, face_encoding)
                best_match_index = np.argmin(face_distances)
                if matches[best_match_index]:
                    print("TRY TO AUTH")
                    a = lock.acquire()
                    authorized = 1
                    lock.release()


                    name = known_face_names[best_match_index]

                face_names.append(name)

        process_this_frame = not process_this_frame

        #display the results I DONT NEED TO DO THIS FOR REAL SERVER
        for(top, right, bottom, left), name in zip(face_locations, face_names):
            top *= 4
            right *= 4
            bottom *= 4
            left *= 4

            cv2.rectangle(frame, (left, top), (right, bottom), (0, 0, 255), 2)

            cv2.rectangle(frame, (left, bottom - 35), (right, bottom), (0, 0, 255), cv2.FILLED)
            font = cv2.FONT_HERSHEY_DUPLEX
            cv2.putText(frame, name, (left + 6, bottom - 6), font, 1.0, (255, 255, 255), 1)

        cv2.imshow('Video', frame)

        if cv2.waitKey(1) == ord("q"):
            break

    capture.release()
    cv2.destroyAllWindows()


face_rec_thread = Thread(target=faceProcessing)
face_rec_thread.start()
