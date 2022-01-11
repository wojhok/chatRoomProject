from posixpath import split
import socket
from threading import *
import threading
from time import sleep
import sys
IP = "127.0.0.1"
PORT = 4455
ADDR = (IP, PORT)
SIZE = 1024
FORMAT = "utf-8"
MSG_GLOBAL = ""
KICKED_GLOBAL = 0
from kivy.app import App
from kivy.uix.label import Label
from kivy.uix.gridlayout import GridLayout
from kivy.uix.textinput import TextInput
from kivy.uix.button import Button
from kivy.uix.screenmanager import ScreenManager, Screen
from kivy.core.window import Window
from kivy.clock import Clock
from kivy.uix.scrollview import ScrollView



class ScrollLabel(ScrollView):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.layout = GridLayout(cols=1, size_hint_y = None)
        self.add_widget(self.layout)

        self.chat_history = Label(size_hint_y=None, markup=True)
        self.scroll_to_point = Label()

        self.layout.add_widget(self.chat_history)
        self.layout.add_widget(self.scroll_to_point)
    
    def update_chat_history(self, message):
        self.chat_history.text += '\n' + message
        
        self.layout.height = self.chat_history.texture_size[1] + 15
        self.chat_history.height = self.chat_history.texture_size[1]
        self.chat_history.text_size = (self.chat_history.width*0.98, None)

        self.scroll_to(self.scroll_to_point)
    
    def update_chat_history_layout(self, _= None):
        self.layout.height = self.chat.history.texture_size[1] + 15
        self.chat_history.height = self.chat_history.texture_size[1]
        self.chat_history.text_size = (self.chat_history.width * 0.98, None)

class ConnectPage(GridLayout):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.cols = 2
        self.add_widget(Label(text="Username:"))

        self.username = TextInput(multiline=False)
        self.add_widget(self.username)

        self.add_widget(Label(text= "Enter Room ID you want to choose or type new to create one: "))
        
        self.room = TextInput(multiline=False)
        self.add_widget(self.room)


        self.join =Button(text= "join")
        self.join.bind(on_press=self.joinButton)
        self.add_widget(Label())
        self.add_widget(self.join)

    def joinButton(self, instance):
        username = self.username.text
        room = self.room.text

        print(f"Attemting to join {username},{room}")
        Clock.schedule_once(self.connect, 1)

    def connect(self, _):
        
        
        username = self.username.text
        room = self.room.text
        chatApp.client.connect(ADDR)
        chatApp.client.send(username.encode(FORMAT))
        sleep(1)
        chatApp.client.send(room.encode(FORMAT))

        chatApp.create_chat_page()
        chatApp.screenManager.current = "Chat"
        

class ChatPage(GridLayout):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.cols = 1
        self.rows = 2

        self.history = ScrollLabel(height=Window.size[1]*0.9, size_hint_y=None)
        self.add_widget(self.history)

        self.new_message = TextInput(width=Window.size[0]*0.8, size_hint_x=None, multiline=False)
        self.send = Button(text="Send")
        self.send.bind(on_press=self.send_message)

        bottom_line = GridLayout(cols=2)
        bottom_line.add_widget(self.new_message)
        bottom_line.add_widget(self.send)
        self.add_widget(bottom_line)

        Window.bind(on_key_down=self.on_key_down)
        Clock.schedule_once(self.focus_text_input, 1)
        t1 = threading.Thread(target=self.recive_messeges, args=[chatApp.client])
        wynik = t1.start()

    
    def on_key_down(self, instance, keyboard, keycode, text, modifiers):
        if keycode == 40:
            self.send_message(None)
    
    def send_message(self, _):
        if chatApp.exit:
            sys.exit()
        message = self.new_message.text
        self.new_message.text = ""
        if message == "kicked":
                return
        if message:
            chatApp.client.send(message.encode(FORMAT))
        if message == "exit":
            chatApp.client.close()
            sys.exit()
        
        
        Clock.schedule_once(self.focus_text_input,0.1)

    def focus_text_input(self, _):
        self.new_message.focus = True
    
    def incoming_message(self, username, message):
        data = chatApp.client.recv(SIZE).decode(FORMAT)
        

    def recive_messeges(self,client):
        while(True):
            data = client.recv(SIZE).decode(FORMAT)  #DATA FROM SERVER
            if self.new_message.text == exit:
                chatApp.client.close()
                chatApp.exit =1
                sys.exit()
            message = str(data)
            self.history.update_chat_history(f"{message}")
            sleep(1)
            if message == "kicked":
                chatApp.client.send("exit".encode(FORMAT))
                sleep(1)
                chatApp.client.close()
                chatApp.exit =1
                sys.exit()
    

class ChatApp(App):
    def build(self):
        self.client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.screenManager = ScreenManager()
        self.exit = 0
        self.connectPage = ConnectPage()
        screen = Screen(name="Connect")
        screen.add_widget(self.connectPage)
        self.screenManager.add_widget(screen)
        
        return self.screenManager
    
    def create_chat_page(self):
        self.chatPage = ChatPage()
        screen = Screen(name="Chat")
        screen.add_widget(self.chatPage)
        self.screenManager.add_widget(screen)
        

    

if __name__ == "__main__":
    chatApp = ChatApp()
    chatApp.run()
    # main()
    