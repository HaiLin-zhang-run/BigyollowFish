@echo off
echo ==============================================
echo 正在为您配置 PoinTr 深度学习训练环境...
echo ==============================================

cd /d "%~dp0"

if not exist "3D_rebuild\Scripts\activate.bat" (
    echo [错误] 找不到 3D_rebuild 虚拟环境！
    pause
    exit /b
)

call 3D_rebuild\Scripts\activate
echo [成功] 虚拟环境已激活！

echo 正在安装 PyTorch 和 CUDA 支持包 (这可能需要几分钟，请耐心等待)...
pip install torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cu121

echo 正在安装 PoinTr 官方依赖...
cd PoinTr
pip install -r requirements.txt
pip install open3d h5py termcolor

echo ==============================================
echo [成功] 深度学习框架及依赖全部安装完毕！
echo ==============================================
pause
