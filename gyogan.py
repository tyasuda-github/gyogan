#-*- using:utf-8 -*-
import time
import math
import tkinter as tk
from tkinter import filedialog
from PIL import Image, ImageTk, ImageOps



class Application(tk.Frame):
	def __init__(self, master = None):
		super().__init__(master)
		self.pack()

		self.master.title("python で球に貼り付け")	# ウィンドウタイトル
		self.master.geometry("1000x10")				# ウィンドウサイズ(幅x高)
		
		# メニューの作成
		self.create_menu()



	def create_menu(self):
		# メニューバーの作成
		menubar = tk.Menu(self)

		# ファイル
		menu_file = tk.Menu(menubar, tearoff = False)
		menu_file.add_command(label = "画像ファイルを開く", command = self.menu_file_open_click, accelerator = "Ctrl+O")
		menu_file.add_separator()	# 仕切り線
		menu_file.add_command(label = "終了", command = self.master.destroy)

		# ショートカットキーの関連付け
		menu_file.bind_all("<Control-o>", self.menu_file_open_click)

		# メニューバーに各メニューを追加
		menubar.add_cascade(label = "ファイル", menu = menu_file)

		# 親ウィンドウのメニューに、作成したメニューバーを設定
		self.master.config(menu = menubar)



	def menu_file_open_click(self, event = None):
		filename = filedialog.askopenfilename(
			title = "ファイルを開く",
			filetypes = [("Image file", ".bmp .png .jpg .tif"), ("*", ".*")],	# ファイルフィルタ
			initialdir = "./"	# 自分自身のディレクトリ
			)

		# 画像を加工して表示
		self.disp_image(filename)



	def disp_image(self, filename):
		# 画像を加工して表示
		if not filename:
			return

		# PIL.Imageで開く
		pil_image = Image.open(filename)
		image_width, image_height = pil_image.size
		print(filename, end="\t")

		# ビューワ表示
		pil_image.show()
		start_time = time.time()


		# 画像を加工
		image_width_half	= int(image_width / 2)
		image_height_half	= int(image_height / 2)
#		print(image_height_half, image_width_half,  sep="\t")

		r_max = math.sqrt(image_width_half * image_width_half + image_height_half * image_height_half)
		s_max = r_max * 2 / math.pi
#		print(r_max, s_max,  sep="\t")

		pil_image_resized = Image.new("RGB", (2 * image_width_half, 2 * image_height_half), color = "#808080")
		pil_image_resized.putpixel((image_width_half, image_height_half), pil_image.getpixel((image_width_half, image_height_half)))

		# range は 1 から始まるらしいよ（0 が含まれない）
		for y in range(0, image_height_half):
			for x in reversed(range(y, image_width_half, 1)):
#				print(y, x,  sep="\t", end="\t")

				if x < y:
#					print("break-1")
					break

				r = math.sqrt(x * x + y * y)
				s = s_max * math.sin(r / s_max)
#				print(r, s, sep="\t", end="\t")

				if int(s) == 0:	# ちょい適当
#					print("break-2")
					break

				u = int(x / r * s)
				v = int(y / r * s)
#				print(v, u,  sep="\t", end="\t")

				pil_image_resized.putpixel((image_width_half + u, image_height_half + v), pil_image.getpixel((image_width_half + x, image_height_half + y)))
				pil_image_resized.putpixel((image_width_half + u, image_height_half - v), pil_image.getpixel((image_width_half + x, image_height_half - y)))
				pil_image_resized.putpixel((image_width_half - u, image_height_half + v), pil_image.getpixel((image_width_half - x, image_height_half + y)))
				pil_image_resized.putpixel((image_width_half - u, image_height_half - v), pil_image.getpixel((image_width_half - x, image_height_half - y)))

				if image_height_half <= x:
#					print("continue-1")
					continue

				if image_width_half <= y:
#					print("continue-2")
					continue

				pil_image_resized.putpixel((image_width_half + v, image_height_half + u), pil_image.getpixel((image_width_half + y, image_height_half + x)))
				pil_image_resized.putpixel((image_width_half + v, image_height_half - u), pil_image.getpixel((image_width_half + y, image_height_half - x)))
				pil_image_resized.putpixel((image_width_half - v, image_height_half + u), pil_image.getpixel((image_width_half - y, image_height_half + x)))
				pil_image_resized.putpixel((image_width_half - v, image_height_half - u), pil_image.getpixel((image_width_half - y, image_height_half - x)))

#				print("eol")



		# ビューワ表示
		pil_image_resized.show()
		elapsed_time = time.time() - start_time
		print(elapsed_time, "[sec]", sep=" ")
		
		pil_image.close()
		pil_image_resized.close()



if __name__ == "__main__":
	root = tk.Tk()
	app = Application(master = root)

	# 画像画像を加工して表示（起動時に一発やってみせる）
	filename = "gyogan.png"
	app.disp_image(filename)

	app.mainloop()
