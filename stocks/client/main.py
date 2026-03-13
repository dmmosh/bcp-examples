import kivy 
from kivy.app import App
from kivy.uix.boxlayout import BoxLayout
import random
import os
import sys
# Add the parent directory to the system path
# # Calculate the path two directories up from the current script's location
# current_dir = os.path.dirname(os.path.abspath(__file__))
# parent_dir = os.path.dirname(current_dir)
# double_parent_dir = os.path.dirname(parent_dir)

# # Add the double previous directory to the system path
# sys.path.append(double_parent_dir)
# from header import *
        

kivy.require('1.9.1')


class MyRoot(BoxLayout):
    def __init__(self):
        super(MyRoot,self).__init__()
        
    def generate_number(self):
        self.random_label.text = str(random.randint(0,1000))

class NeuralRandom(App): # kv file needs to be the same name as the class (all lowercase)
    def build(self):
        return MyRoot()
    
    
neuralRandom = NeuralRandom()
neuralRandom.run()