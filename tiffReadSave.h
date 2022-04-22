#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <fstream>
#include <sstream>
using namespace std;
#define MC_GET_16(__data__) ((__data__ & 0xFF) << 8) | (__data__ >> 8)
#define MC_GET_64(__data__) ((__data__ & 0xFF) << 24) | ((__data__ & 0xFF00) << 8) | ((__data__ & 0xFF0000) >> 8) | ((__data__ & 0xFF000000) >> 24)

template<typename T>
struct complex {
	T re;
	T im;
	complex(): re(T{}), im(T{}) {};
	complex(T re_) :re(re_), im(T{}) {};
	complex(T re_, T im_) :re(re_), im(im_) {};
};
typedef complex<float> cor;
typedef struct
{
	uint16_t endian; // 字节顺序标志位,值为II或者MM:II表示小字节在前,又称为little-endian,MM表示大字节在前,又称为big-endian
	uint16_t magic;  // TIFF的标志位，一般都是42
	uint32_t ifdOffset; // 第一个IFD的偏移量,可以在任意位置,但必须是在一个字的边界,也就是说必须是2的整数倍
}IFH;
typedef struct
{
	uint16_t tag;  // 此TAG的唯一标识
	uint16_t type; // 数据类型
	uint32_t size; // 数量,通过类型和数量可以确定存储此TAG的数据需要占据的字节数
	uint32_t valOffset;
}DE;

typedef struct
{
	uint16_t n; // 表示此IFD包含了多少个DE,假设数目为n
	DE* p; // n个DE
	uint32_t nextIfdOffset; // 下一个IFD的偏移量,如果没有则置为0
}IFD;
typedef struct imgInfo
{
	uint32_t width; // 表示图像宽度
	uint32_t height;  // 表示图像高度
	uint8_t channel;  // 表示图像通道数
	uint64_t ImageStart;// 图像数据起始地址存储位置相对于文件开始的位置val的保存位置
	uint8_t ImageStartTypeSize;//位深
	uint16_t byteNum;
	uint16_t typeLabel;
}imInfo;
/*
* 测试机器是大端(big endian:返回false)还是小端(little endian:返回true)
*/
inline bool mashineEndian(void)
{
	uint16_t a = 1;
	uint16_t* p = &a;
	return *((uint8_t*)p) == a;
}
class TiffImg {
public:
	imInfo _imInfo;
	uint8_t* data;
	IFH _IFH;
	IFD _IFD;
	TiffImg(string path);
	void save(string savePath= "defaultSave.tif");
	~TiffImg() {
		save();
		free(data);
		data = nullptr;
		free(_IFD.p);
		_IFD.p = nullptr;
	}
	template<typename T>
	T& operator()(T,uint32_t row, uint32_t col, uint8_t depth=1 ){
		uint64_t sum = ((row - 1) * _imInfo.width * _imInfo.channel + (col - 1) * _imInfo.channel + (depth - 1)) * _imInfo.ImageStartTypeSize;
		return *((T*)(data + sum));
	}
private:
	string Path;
	bool EndianTrans;
	bool read();

	const uint16_t littleEndian = 0x4949;
	const uint16_t bigEndian = 0x4d4d;
};
TiffImg::TiffImg(string path) {
	Path = path;
	if (read()) {
		ifstream frd(Path, ios::in | ios::binary);
		if (frd) {//文件打开成功
			frd.seekg(static_cast<long>(_imInfo.ImageStart), ios::beg);
			for (uint32_t row = 0; row < _imInfo.height; ++row) {
				for (uint32_t col = 0; col < _imInfo.width; ++col) {
					for (uint8_t dep = 0; dep < _imInfo.channel; ++dep) {
						uint8_t* temp = data + (row * _imInfo.width * _imInfo.channel + col * _imInfo.channel + dep) * _imInfo.ImageStartTypeSize;
						frd.read(reinterpret_cast<char*>(temp), _imInfo.ImageStartTypeSize);
						if (EndianTrans) {//大小端转换,暂未完成
							;
						}
					}
				}
			}
			frd.close();
		}
	}
};
bool TiffImg::read()
{
	ifstream frd(Path, ios::in | ios::binary);
	if (frd) {//文件打开成功
		frd.read(reinterpret_cast<char*>(&(_IFH.endian)), sizeof(_IFH.endian));
		frd.read(reinterpret_cast<char*>(&(_IFH.magic)), sizeof(_IFH.magic));
		frd.read(reinterpret_cast<char*>(&(_IFH.ifdOffset)), sizeof(_IFH.ifdOffset));
		bool imEndian{ _IFH.endian == littleEndian }; //图像存储方式是大端(false)还是小端(true)
		EndianTrans = (imEndian != mashineEndian());
		if (EndianTrans) {
			_IFH.endian = MC_GET_16(_IFH.endian);
			_IFH.magic = MC_GET_16(_IFH.magic);
			_IFH.ifdOffset = MC_GET_64(_IFH.ifdOffset);
		}
		auto IFD_Start = _IFH.ifdOffset;
		frd.seekg(static_cast<long>(IFD_Start), ios::beg);
		frd.read(reinterpret_cast<char*>(&(_IFD.n)), sizeof(_IFD.n));
		if (EndianTrans) {
			_IFD.n = MC_GET_16(_IFD.n);
		}
		auto DE_n = _IFD.n;
		_IFD.p = (DE*)malloc(sizeof(DE) * DE_n);
		uint32_t DE_val_size{};
		for (uint32_t i = 0; i < DE_n; ++i) {
			frd.read(reinterpret_cast<char*>(&(_IFD.p[i].tag)), sizeof(_IFD.p[i].tag));
			frd.read(reinterpret_cast<char*>(&(_IFD.p[i].type)), sizeof(_IFD.p[i].type));
			frd.read(reinterpret_cast<char*>(&(_IFD.p[i].size)), sizeof(_IFD.p[i].size));
			frd.read(reinterpret_cast<char*>(&(_IFD.p[i].valOffset)), sizeof(_IFD.p[i].valOffset));
			if ((_IFH.endian == littleEndian) != mashineEndian()) {
				_IFD.p[i].tag = MC_GET_16(_IFD.p[i].tag);
				_IFD.p[i].type = MC_GET_16(_IFD.p[i].type);
				_IFD.p[i].size = MC_GET_64(_IFD.p[i].size);
			}

			if (EndianTrans) {
				if (DE_val_size == 1) {
					_IFD.p[i].valOffset = _IFD.p[i].valOffset;
				}
				else if (DE_val_size == 2) {
					_IFD.p[i].valOffset = MC_GET_16(_IFD.p[i].valOffset);
				}
				else if (DE_val_size >= 4) {
					_IFD.p[i].valOffset = MC_GET_64(_IFD.p[i].valOffset);
				}
			}
			switch (_IFD.p[i].tag) {
			case 0x0100: _imInfo.width = _IFD.p[i].valOffset; break;// 图像宽度存储位置
			case 0x0101: _imInfo.height = _IFD.p[i].valOffset; break;// 图像高度存储位置
			case 0x0102: {
				_imInfo.typeLabel = _IFD.p[i].type;
				switch (_IFD.p[i].type) {
				case 1:_imInfo.ImageStartTypeSize = 1;//Byte
				case 2: DE_val_size = (_IFD.p)[i].size; break;//文本类型，7位Ascii码加1位二进制0
				case 3:   _imInfo.ImageStartTypeSize = 2; DE_val_size = (_IFD.p)[i].size * sizeof(uint16_t); break;//2字节
				case 4: _imInfo.ImageStartTypeSize = 4; DE_val_size = (_IFD.p)[i].size * sizeof(uint32_t); break;//4字节
				case 5:   _imInfo.ImageStartTypeSize = 8; DE_val_size = (_IFD.p)[i].size * sizeof(uint64_t); break;//暂不准确：有理数，第一无符号Long为小数，第二个为整数部分
				case 6:  _imInfo.ImageStartTypeSize = 1; DE_val_size = (_IFD.p)[i].size * sizeof(int8_t); break;
				case 8:  _imInfo.ImageStartTypeSize = 2; DE_val_size = (_IFD.p)[i].size * sizeof(int16_t); break;
				case 9:   _imInfo.ImageStartTypeSize = 4; DE_val_size = (_IFD.p)[i].size * sizeof(int32_t); break;
				case 10:  _imInfo.ImageStartTypeSize = 8; DE_val_size = (_IFD.p)[i].size * sizeof(int64_t); break;///暂不准确：有理数，第一Long为小数，第二个为整数部分
				case 11: _imInfo.ImageStartTypeSize = 4; DE_val_size = (_IFD.p)[i].size * sizeof(float); break;
				case 12:  _imInfo.ImageStartTypeSize = 8; DE_val_size = (_IFD.p)[i].size * sizeof(double); break;
				default: _imInfo.ImageStartTypeSize = 1;  DE_val_size = (_IFD.p)[i].size; break;
				}
				break;// 颜色深度  值＝1为单色，＝4为16色，＝8为256色。如果该类型数据个数＞2个，说明是真彩图像
			}
			case 0x0103: break;// 图像是否压缩 05表示压缩
			case 262: {// 表示图像的种类: 0-WhiteisZero 1-BlackisZero 2-RGBImg 3-PaletteColor ...
				_imInfo.channel = _IFD.p[i].valOffset == 2 ? 3 : 1;
				break;
			}
			case 0x0111: {// 图像数据起始地址存储位置相对于文件开始的位置val的保存位置
				_imInfo.ImageStart = _IFD.p[i].valOffset;
				break;
			}
			case 274: break;// 图像坐标系方式: 1[left top] 2[right top] 3[bottom right] ...
			case 277: break;
			case 278: break;// 表示每一个Strip内包含的图像的行数 eg:Img[w][h][w] --> RowsPerStrip=1
			case 0x0117: {
				_imInfo.byteNum = _IFD.p[i].valOffset;
				break; // 图像字节总数，非偶数后面补零
			}
			case 284: break;// 全彩图像每一个像素点排列方式: 1[RGB...RGB]-交叉 2[[R...R],[G...G],[B...B]]-平铺
			default:  break;
			}
		}
		frd.close();
		data =(uint8_t *)malloc(_imInfo.height* _imInfo.width* _imInfo.channel* _imInfo.ImageStartTypeSize);
		return true;
	}
	return false;
}
void TiffImg::save(string savePath) {
	ofstream fd{ savePath, ios::out | ios::binary };
	if (fd.is_open()) {
		fd.seekp(0L,ios::beg);
		if (EndianTrans) {
			uint16_t Endian =(_IFH.endian==littleEndian ? bigEndian: littleEndian);
			fd.write((char*)(& (Endian)), sizeof(_IFH.endian));
		}
		else {
			fd.write((char*)&(_IFH.endian), sizeof(_IFH.endian));
		}
		fd.write((char*)&(_IFH.magic), sizeof(_IFH.magic));
		fd.write((char*)&(_IFH.ifdOffset), sizeof(_IFH.ifdOffset));
		fd.seekp(_IFH.ifdOffset, ios::beg);
		fd.write((char*)&(_IFD.n), sizeof(_IFD.n));
		for (uint16_t i = 0; i < _IFD.n; ++i) {//仅仅适用于有一个IFH的文件
			fd.write((char*)( & (_IFD.p[i].tag)),sizeof(_IFD.p->tag));
			fd.write((char*)(&(_IFD.p[i].type)), sizeof(_IFD.p->type));
			fd.write((char*)(&(_IFD.p[i].size)), sizeof(_IFD.p->size));
			fd.write((char*)(&(_IFD.p[i].valOffset)), sizeof(_IFD.p->valOffset));
		}
		fd.write((char*)(&(_IFD.nextIfdOffset)),sizeof(_IFD.nextIfdOffset)); // 下一个图像文件的入口位置，一般对于单一文件，这里为0
		fd.seekp(_imInfo.ImageStart,ios::beg);
		uint64_t byteNum = _imInfo.height * _imInfo.width * _imInfo.channel * _imInfo.ImageStartTypeSize;
		for (uint64_t i = 0; i < byteNum;++i) {//写图像数据
			fd.write((char*)(&(data[i])), sizeof(data[i]));
		}
		fd.close();
	}

}


