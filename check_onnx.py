import onnxruntime as ort

print("best.onnx")
sess1 = ort.InferenceSession("d:/QPRO/best.onnx")
for i in sess1.get_inputs():
    print("In:", i.name, i.shape)
for o in sess1.get_outputs():
    print("Out:", o.name, o.shape)

print("fish_keypoint.onnx")
sess2 = ort.InferenceSession("d:/QPRO/fish_keypoint.onnx")
for i in sess2.get_inputs():
    print("In:", i.name, i.shape)
for o in sess2.get_outputs():
    print("Out:", o.name, o.shape)
