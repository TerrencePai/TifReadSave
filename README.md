# TifReadSave
Tif文件的读取、修改对应像素、保存。
对应的说明请看我的CSDN博客:

调用示例：
#include"tiffReadSave.h"
#include <iostream>

int main() {
	TiffImg img("test.tif");//img为自定义的变量名
	//img._imInfo结构体保存了图像的高度，宽度，通道数，位深等信息
	img(uint16_t(0),149, 11,1);//这句话表示获取图像的第149行11列的第1个通道的数据(第四个参数缺省为1)
	cout << img(uint16_t(0), 149, 11);//可以看到该表达式可以放在等式右边赋值给其他变量(右值)
	img(uint16_t(0), 149, 11)=3;//可以看到该表达式可以放在等式左边被其他变量所赋值(左值)
	cout << img(uint16_t(0), 149, 11);
	img.save("defaultSave.tif");//调用保存函数保存图像文件，不过忘记保存也没关系，在跑出作用域/程序结束后会自动保存的，默认保存文件名：defaultSave.tif
	//已在matlab比照数据验证过
	return 0;
	//img._imInfo.typeLabel
}
  
  
  /
