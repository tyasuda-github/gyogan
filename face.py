import glob
import cv2

files = glob.glob(r'D:\Desktop\*.jpg')
files = glob.glob(r'D:\AH-4832\gyogan\gyogan.png')
files = glob.glob(r'D:\AH-4832\gyogan\OSU07.jpg')

cascade = cv2.CascadeClassifier(r'C:\Users\tyasuda.TYASUDA-EPSON\AppData\Local\Programs\Python\Python36\Lib\site-packages\cv2\data\haarcascade_frontalface_default.xml')

#capture = cv2.VideoCapture(1)

try:
	for file in files:
		print(file)
		color = cv2.imread(file)
		gray  = cv2.cvtColor(color,cv2.COLOR_BGR2GRAY)

#		front_face_list = cascade.detectMultiScale(gray, minSize=(50,50))
#		front_face_list = cascade.detectMultiScale(gray, scaleFactor=1.3, minNeighbors=5)
		front_face_list = cascade.detectMultiScale(gray, scaleFactor=1.3, minNeighbors=5, minSize=(50,50))
		print(front_face_list)

		if len(front_face_list) == 0:
			print("Failed")
			cv2.waitKey(1000)
			continue

		for (x,y,w,h) in front_face_list:
			cv2.rectangle(color,(x,y),(x+w,y+h),(128,128,128),thickness=4)
			cv2.imshow(file,color)

		cv2.waitKey(10000)

except:
	cv2.destroyAllWindows

