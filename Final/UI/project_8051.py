import sys
import serial
import serial.tools.list_ports
from PyQt5.QtWidgets import QApplication, QMainWindow, QLabel, QVBoxLayout, QPushButton, QWidget, QComboBox, QFileDialog, QTextEdit, QSplitter
from PyQt5.QtGui import QPixmap, QImage
from PyQt5.QtCore import Qt, QBuffer, QCoreApplication, QTimer
import cv2
import numpy as np
from PIL import Image
import io
from itertools import groupby
import time

def remove_consecutive_duplicates(rows, n):
    result = []
    n_consecutive_duplicates = 1

    for key, group in groupby(rows):
        consecutive_duplicates = list(group)
        if len(consecutive_duplicates) >= n:
            result.extend(consecutive_duplicates)
            n_consecutive_duplicates += 1
        else:
            n_consecutive_duplicates = 1
    return result

def remove_duplicates(rows):
    result = []
    result.append(rows[0])
    for i in range(1, len(rows)):
        if not np.array_equal(rows[i], rows[i - 1]):
            result.append(rows[i])
    return result
        
class ImageToHexConverter(QMainWindow):
    def __init__(self):
        super().__init__()
        self.video_capture = None
        self.initUI()
        self.serial_port = serial.Serial()
        
    def initUI(self):
        self.setWindowTitle('Image to Hex Converter')
        self.setGeometry(100, 100, 800, 400)

        splitter = QSplitter(Qt.Horizontal)

        # Cột hiển thị hình ảnh
        imageColumn = QWidget()
        self.imageLabel = QLabel()
        self.imageLabel.setAlignment(Qt.AlignCenter)
        self.imageLabel.setMinimumWidth(300)  # Đặt kích thước tối thiểu cho cột hình ảnh
        imageLayout = QVBoxLayout()
        imageLayout.addWidget(self.imageLabel)
        btnLoadLayout = QVBoxLayout()
        btnLoad = QPushButton('Load Image')
        btnLoad.clicked.connect(self.loadImage)
        btnLoadLayout.addWidget(btnLoad)
        imageLayout.addLayout(btnLoadLayout)
        btnConvertLayout = QVBoxLayout()
        btnConvert = QPushButton('Convert Image to Hex')
        btnConvert.clicked.connect(self.handleImage)
        btnConvertLayout.addWidget(btnConvert)
        imageLayout.addLayout(btnConvertLayout)
        imageColumn.setLayout(imageLayout)
        splitter.addWidget(imageColumn)

        # Cột hiển thị mã hex
        hexColumn = QWidget()
        self.hexTextEdit = QTextEdit()
        hexLayout = QVBoxLayout()
        hexLayout.addWidget(self.hexTextEdit)
        hexColumn.setLayout(hexLayout)
        splitter.addWidget(hexColumn)

        # Thêm nút nhấn "Import"
        btnImport = QPushButton('Import')
        btnImport.clicked.connect(self.importTo8051)
        # Thêm nút nhấn vào layout của cột hiển thị mã hex
        hexLayout.addWidget(btnImport)
        # Thêm cột chia cho cửa sổ chính
        self.setCentralWidget(splitter)

        # Thêm QComboBox để chọn cổng COM
        self.comComboBox = QComboBox()
        self.populateComPorts()
        hexLayout.addWidget(self.comComboBox)
        hexColumn.setLayout(hexLayout)
        splitter.addWidget(hexColumn)

        # Thêm nút "Load Video" và "Convert Video"
        btnLoadVideo = QPushButton('Load Video')
        btnLoadVideo.clicked.connect(self.loadVideo)
        hexLayout.addWidget(btnLoadVideo)
        btnConvertVideo = QPushButton('Convert Video to Hex')
        btnConvertVideo.clicked.connect(self.convertVideo)
        hexLayout.addWidget(btnConvertVideo)

        # Thêm cột chia cho cửa sổ chính
        self.setCentralWidget(splitter)

        # Biến lưu trữ ảnh gốc
        self.original_image = None

        self.com_refresh_timer = QTimer(self)
        self.com_refresh_timer.timeout.connect(self.updateComPorts)
        self.com_refresh_timer.start(1000)

    def populateComPorts(self):
        com_ports = [port.device for port in serial.tools.list_ports.comports()]
        self.comComboBox.addItems(com_ports)

    def updateComPorts(self):
        # Cập nhật danh sách cổng COM
        com_ports = [port.device for port in serial.tools.list_ports.comports()]
        current_port = self.comComboBox.currentText()

        # Xóa danh sách cũ
        self.comComboBox.clear()

        # Thêm danh sách cổng COM mới
        self.comComboBox.addItems(com_ports)

        # Chọn lại cổng COM trước đó nếu nó vẫn tồn tại trong danh sách mới
        if current_port in com_ports:
            self.comComboBox.setCurrentText(current_port)

    def loadImage(self):
        options = QFileDialog.Options()
        filePath, _ = QFileDialog.getOpenFileName(self, "Open Image", "", "Image Files (*.png *.jpg *.bmp);;All Files (*)", options=options)
        if filePath:
            # Load ảnh gốc
            self.original_image = QImage(filePath)

            # Hiển thị ảnh gốc
            pixmap = QPixmap.fromImage(self.original_image)
            new_size = pixmap.scaled(300, 200, Qt.KeepAspectRatio)
            self.imageLabel.setPixmap(new_size)
    def loadVideo(self):
        options = QFileDialog.Options()
        filePath, _ = QFileDialog.getOpenFileName(self, "Open Video", "", "Video Files (*.mp4 *.avi);;All Files (*)", options=options)
        if filePath:
            self.video_capture = cv2.VideoCapture(filePath)
            while True:
                ret, frame = self.video_capture.read()
                if not ret:
                    break
                # Convert frame to QImage
                image = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
                height, width, channel = image.shape
                bytes_per_line = 3 * width
                qimage = QImage(image.data, width, height, bytes_per_line, QImage.Format_RGB888)

                # Display the frame in QLabel
                pixmap = QPixmap.fromImage(qimage)
                new_size = pixmap.scaled(300, 200, Qt.KeepAspectRatio)
                self.imageLabel.setPixmap(new_size)

                # Optionally add a delay to control the video playback speed
                QCoreApplication.processEvents()
                time.sleep(0.02)
             
    def convertVideo(self):
        if self.video_capture is not None:
            frame_list = []
            fps = self.video_capture.get(cv2.CAP_PROP_FPS)
            while True:
                ret, frame = self.video_capture.read()
                if not ret:
                    break
                gray_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
                resized_image = cv2.resize(gray_frame, (8, 8))
                binary_array = np.where(resized_image > 192, 1, 0)
                hex_array = []
                for col in zip(*binary_array):
                    decimal = int(''.join(map(str, col)), 2)
                    if decimal == 0:
                        hex_value = '00'
                        hex_array.append(hex_value)
                    else:
                      if (decimal < 16):
                          hex_value = "0" + hex(decimal)[2:].upper()
                          hex_array.append(hex_value)
                      else:
                          hex_value = hex(decimal)[2:].upper()
                          hex_array.append(hex_value)

                frame_list.append(' '.join(hex_array))
            result = []
            result = remove_duplicates(remove_consecutive_duplicates(frame_list, int(fps - 5)))
            frame_text = '\n'.join(result)
            size = len(result) * 8
            if(size >= 4096):
                size_hex_value = hex(size)[2:].upper()
            elif(size >= 256):
                size_hex_value = "0" + hex(size)[2:].upper()
            elif(size >= 16):
                size_hex_value = "00" + hex(size)[2:].upper()
            else:
                size_hex_value = "000" + hex(size)[2:].upper()
            size_hex_value_swaped = size_hex_value[2:] + " " + size_hex_value[:2] + " "
            header = "2A 2A 2A "
            header_reserve = " 00 00"
            if(fps > 16):
                fps_string = hex(int(fps))[2:].upper()
            else:
                fps_string = "0" + hex(fps)[2:].upper()
            hex_text = header + size_hex_value_swaped + fps_string + header_reserve + '\n' + frame_text

            # Hiển thị mã hex trong QTextEdit
            self.hexTextEdit.setPlainText(hex_text)
            # Release video capture when video ends
            self.video_capture.release()
            
    def handleImage(self):    
        if self.original_image is not None:
                # Chuyển đổi QImage thành ảnh OpenCV
              cv_image = self.qimage_to_cvimage(self.original_image)

              # Chuyển đổi ảnh sang ảnh xám
              gray_image = cv2.cvtColor(cv_image, cv2.COLOR_BGR2GRAY)
              resized_image = cv2.resize(gray_image, (8, 8))
              binary_array = np.zeros((8, 8), dtype=np.uint8)
              binary_array = np.where(resized_image > 192, 1, 0)
              hex_array = []
              for col in zip(*binary_array):
                  decimal = int(''.join(map(str, col)), 2)
                  if decimal == 0:
                      hex_value = '00'
                      hex_array.append(hex_value)
                  else:
                      if (decimal < 16):
                          hex_value = "0" + hex(decimal)[2:].upper()
                          hex_array.append(hex_value)
                      else:
                          hex_value = hex(decimal)[2:].upper()
                          hex_array.append(hex_value)
            # Join the elements of hex_array into a single string
        size = len(hex_array)
        if(size >= 4096):
            size_hex_value = hex(size)[2:].upper()
        elif(size >= 256):
            size_hex_value = "0" + hex(size)[2:].upper()
        elif(size >= 16):
            size_hex_value = "00" + hex(size)[2:].upper()
        else:
            size_hex_value = "000" + hex(size)[2:].upper()
        size_hex_value_swaped = size_hex_value[2:] + " " + size_hex_value[:2] + " "
        header = "2A 2A 2A "
        header_reserve = "00 00"
        fps = "10 "    #Chọn ngẫu nhiên thôi
        hex_text = header + size_hex_value_swaped + fps + header_reserve + '\n' + ' '.join(hex_array)
        # Hiển thị mã hex trong QTextEdit
        self.hexTextEdit.setPlainText(hex_text)
              
    def qimage_to_cvimage(self, qimage):
        """Chuyển đổi QImage thành ảnh OpenCV."""
        buffer = QBuffer()
        buffer.open(QBuffer.ReadWrite)
        qimage.save(buffer, "PNG")
        buffer.seek(0)
        pil_image = Image.open(io.BytesIO(buffer.data()))
        cv_image = np.array(pil_image)
        return cv_image
    
    
    def importTo8051(self):
        # Lấy mã hex từ QTextEdit
        hex_data = self.hexTextEdit.toPlainText()
        # Gửi mã hex sang 8051 qua giao tiếp serial
        self.sendTo8051(hex_data)
        
    def sendTo8051(self, hex_data):
        try:
            # Configure the serial port
            self.serial_port.port = self.comComboBox.currentText()
            self.serial_port.baudrate = 9600  # Set the baud rate as needed
            self.serial_port.timeout = 1  # Set the timeout as needed
            self.serial_port.open()

            # Send hex_data to the 8051
            byte_data = bytes.fromhex(hex_data.replace(' ', ''))
            self.serial_port.write(byte_data)

        except serial.SerialException as e:
            print(f"Error: {e}")

        finally:
            # Close the serial port
            if self.serial_port.is_open:
                self.serial_port.close()


if __name__ == '__main__':
    app = QApplication(sys.argv)
    mainWindow = ImageToHexConverter()
    mainWindow.show()
    sys.exit(app.exec_())