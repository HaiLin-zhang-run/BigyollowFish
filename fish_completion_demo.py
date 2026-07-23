import numpy as np
import open3d as o3d
import copy

def auto_complete_fish_symmetry(pcd, symmetry_axis='x'):
    """
    自动补全基于对称性的半边鱼点云数据。
    
    参数:
    pcd (open3d.geometry.PointCloud): 输入的原始半边点云
    symmetry_axis (str): 对称面法向量所在的轴。支持 'x', 'y', 'z', 或 'auto'（自动检测最薄的轴）。
    """
    print("1. 原始点云数量:", len(pcd.points))
    
    # 自动利用 RANSAC 提取最大的平坦面（鱼的扫描切面）作为对称面
    print("正在使用 RANSAC 算法检测底部的平坦切面作为对称面...")
    bbox = pcd.get_axis_aligned_bounding_box()
    diagonal = np.linalg.norm(bbox.get_extent())
    distance_threshold = diagonal * 0.01 # 根据物体大小自适应设定误差阈值
    
    plane_model, inliers = pcd.segment_plane(distance_threshold=distance_threshold,
                                             ransac_n=3,
                                             num_iterations=2000)
    [a, b, c, d] = plane_model
    print(f"检测到平面方程: {a:.3f}x + {b:.3f}y + {c:.3f}z + {d:.3f} = 0")
    
    # 构建法向量并归一化
    n = np.array([a, b, c])
    n = n / np.linalg.norm(n)
    a, b, c = n
    
    # 构建 Householder 镜像变换矩阵
    M = np.array([
        [1 - 2*a*a,  -2*a*b,    -2*a*c,    -2*a*d],
        [-2*a*b,     1 - 2*b*b, -2*b*c,    -2*b*d],
        [-2*a*c,     -2*b*c,    1 - 2*c*c, -2*c*d],
        [0,          0,         0,         1]
    ])
    
    # 复制点云并应用镜像矩阵
    pcd_mirrored = copy.deepcopy(pcd)
    pcd_mirrored.transform(M)
    
    # 修正法向：由于是镜像，法线需要反转以保持朝向外部的一致性
    if pcd_mirrored.has_normals():
        normals = np.asarray(pcd_mirrored.normals)
        pcd_mirrored.normals = o3d.utility.Vector3dVector(-normals)
        
    # 为镜像的点云上色（红色）
    pcd_mirrored.paint_uniform_color([1, 0, 0])
    
    print("2. 镜像点云生成完毕.")

    # 3. 自动配准缝合 (ICP Registration)
    # 在实际情况中，由于鱼的姿态或扫描误差，单纯的镜像可能无法完美对齐
    # 使用 ICP (Iterative Closest Point) 进行微调对齐
    threshold = 0.02
    trans_init = np.eye(4)
    print("3. 正在运行 ICP 自动微调对齐...")
    reg_p2p = o3d.pipelines.registration.registration_icp(
        pcd_mirrored, pcd, threshold, trans_init,
        o3d.pipelines.registration.TransformationEstimationPointToPoint()
    )
    
    pcd_mirrored.transform(reg_p2p.transformation)
    
    # 4. 合并点云 (Merge)
    completed_pcd = pcd + pcd_mirrored
    
    # 5. 下采样与去重 (Downsample to remove overlapping points at the seam)
    completed_pcd = completed_pcd.voxel_down_sample(voxel_size=0.005)
    
    print("4. 自动补全完成，合并后点云数量:", len(completed_pcd.points))
    return completed_pcd

if __name__ == "__main__":
    print("=== 鱼类3D点云自动补全演示程序 ===")
    print("温馨提示: 运行此脚本需要安装 open3d: pip install open3d")
    
    # 读取用户指定的鱼类点云文件
    file_path = r"D:\QPRO\fish_records.csv\2026-07-23\20260723_102230\fish_20260723_102230_cloud.ply"
    print(f"正在加载点云文件: {file_path}")
    pcd_mock = o3d.io.read_point_cloud(file_path)
    
    if not pcd_mock.has_points():
        print("错误: 无法读取点云文件，请检查路径是否正确或文件是否损坏。")
        exit()
        
    # 如果原点云没有法向量，估算一下法向量
    if not pcd_mock.has_normals():
        pcd_mock.estimate_normals()
        
    pcd_mock.paint_uniform_color([0, 1, 0]) # 绿色代表原始数据
    
    print("展示原始残缺的半边模型 (绿色)...")
    o3d.visualization.draw_geometries([pcd_mock], window_name="Original Half")
    
    # 调用自动化补全算法（改为 'auto' 自动寻找对称轴）
    completed_fish = auto_complete_fish_symmetry(pcd_mock, symmetry_axis='auto')
    
    print("展示自动补全后的完整模型 (绿色为原始，红色为AI补全)...")
    o3d.visualization.draw_geometries([completed_fish], window_name="Completed Model")
    
    # o3d.io.write_point_cloud("completed_fish.ply", completed_fish)
    print("流程结束。")
