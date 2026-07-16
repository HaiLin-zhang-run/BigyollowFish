import sys
import glob
import cv2
import numpy as np
import onnxruntime as ort

internal_path = r"C:\Users\18772\Desktop\暑假任务\大黄鱼\CeFish\CeFish\_internal"
sys.path.insert(0, internal_path)

model_path = r"D:\QPRO\fish_keypoint.onnx"
img_files = glob.glob(r"C:\Users\18772\Desktop\暑假任务\大黄鱼\CeFish\test_pics\*.jpg")
img_path = img_files[0]
print(f"Testing keypoint on image: {img_path}")

img = cv2.imdecode(np.fromfile(img_path, dtype=np.uint8), -1)

targetW, targetH = 640, 640

# Direct Resize (Let's see if this works, if not we'll use letterbox)
resized = cv2.resize(img, (targetW, targetH))
resized = cv2.cvtColor(resized, cv2.COLOR_BGR2RGB)
resized = resized.astype(np.float32) / 255.0
resized = (resized - np.array([0.485, 0.456, 0.406])) / np.array([0.229, 0.224, 0.225])
blob = np.transpose(resized, (2, 0, 1))
blob = np.expand_dims(blob, axis=0).astype(np.float32)

sess = ort.InferenceSession(model_path)
inputs = {sess.get_inputs()[0].name: blob}
outputs = sess.run(None, inputs)

# Outputs should be: [heatmaps, unsqueeze, topk_scores, topk_indices]
# Let's check outputs length
print([o.shape for o in outputs])

heat_shape = outputs[0].shape
scores = outputs[2] # shape (1, 17, 30)
indices = outputs[3] # shape (1, 17, 30)

heatW = heat_shape[-1]
heatH = heat_shape[-2]

vis_img = img.copy()
for kp in range(17):
    score = scores[0, kp, 0]
    idx = indices[0, kp, 0]
    hx = idx % heatW
    hy = idx // heatW
    
    # decode direct resize
    orig_x = int((hx + 0.5) / heatW * img.shape[1])
    orig_y = int((hy + 0.5) / heatH * img.shape[0])
    
    cv2.circle(vis_img, (orig_x, orig_y), 5, (0, 0, 255), -1)
    cv2.putText(vis_img, str(kp+1), (orig_x, orig_y-10), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0,0,255), 1)

cv2.imwrite(r"D:\QPRO\test_kps.jpg", vis_img)
print("Saved D:\\QPRO\\test_kps.jpg")
