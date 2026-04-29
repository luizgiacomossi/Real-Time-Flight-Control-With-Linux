import os
from svglib.svglib import svg2rlg
from reportlab.graphics import renderPDF

def convert_svgs_to_pdf():
    current_directory = os.getcwd()
    
    files = [f for f in os.listdir(current_directory) if f.lower().endswith('.svg')]
    
    if not files:
        return

    for file_name in files:
        try:
            drawing = svg2rlg(file_name)
            output_name = f"{os.path.splitext(file_name)[0]}.pdf"
            renderPDF.drawToFile(drawing, output_name)
        except Exception as e:
            pass

if __name__ == "__main__":
    convert_svgs_to_pdf()