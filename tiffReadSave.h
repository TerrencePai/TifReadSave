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
	uint16_t endian; // �ֽ�˳���־λ,ֵΪII����MM:II��ʾС�ֽ���ǰ,�ֳ�Ϊlittle-endian,MM��ʾ���ֽ���ǰ,�ֳ�Ϊbig-endian
	uint16_t magic;  // TIFF�ı�־λ��һ�㶼��42
	uint32_t ifdOffset; // ��һ��IFD��ƫ����,����������λ��,����������һ���ֵı߽�,Ҳ����˵������2��������
}IFH;
typedef struct
{
	uint16_t tag;  // ��TAG��Ψһ��ʶ
	uint16_t type; // ��������
	uint32_t size; // ����,ͨ�����ͺ���������ȷ���洢��TAG��������Ҫռ�ݵ��ֽ���
	uint32_t valOffset;
}DE;

typedef struct
{
	uint16_t n; // ��ʾ��IFD�����˶��ٸ�DE,������ĿΪn
	DE* p; // n��DE
	uint32_t nextIfdOffset; // ��һ��IFD��ƫ����,���û������Ϊ0
}IFD;
typedef struct imgInfo
{
	uint32_t width; // ��ʾͼ����
	uint32_t height;  // ��ʾͼ��߶�
	uint8_t channel;  // ��ʾͼ��ͨ����
	uint64_t ImageStart;// ͼ��������ʼ��ַ�洢λ��������ļ���ʼ��λ��val�ı���λ��
	uint8_t ImageStartTypeSize;//λ��
	uint16_t byteNum;
	uint16_t typeLabel;
}imInfo;
/*
* ���Ի����Ǵ��(big endian:����false)����С��(little endian:����true)
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
		if (frd) {//�ļ��򿪳ɹ�
			frd.seekg(static_cast<long>(_imInfo.ImageStart), ios::beg);
			for (uint32_t row = 0; row < _imInfo.height; ++row) {
				for (uint32_t col = 0; col < _imInfo.width; ++col) {
					for (uint8_t dep = 0; dep < _imInfo.channel; ++dep) {
						uint8_t* temp = data + (row * _imInfo.width * _imInfo.channel + col * _imInfo.channel + dep) * _imInfo.ImageStartTypeSize;
						frd.read(reinterpret_cast<char*>(temp), _imInfo.ImageStartTypeSize);
						if (EndianTrans) {//��С��ת��,��δ���
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
	if (frd) {//�ļ��򿪳ɹ�
		frd.read(reinterpret_cast<char*>(&(_IFH.endian)), sizeof(_IFH.endian));
		frd.read(reinterpret_cast<char*>(&(_IFH.magic)), sizeof(_IFH.magic));
		frd.read(reinterpret_cast<char*>(&(_IFH.ifdOffset)), sizeof(_IFH.ifdOffset));
		bool imEndian{ _IFH.endian == littleEndian }; //ͼ��洢��ʽ�Ǵ��(false)����С��(true)
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
			case 0x0100: _imInfo.width = _IFD.p[i].valOffset; break;// ͼ���ȴ洢λ��
			case 0x0101: _imInfo.height = _IFD.p[i].valOffset; break;// ͼ��߶ȴ洢λ��
			case 0x0102: {
				_imInfo.typeLabel = _IFD.p[i].type;
				switch (_IFD.p[i].type) {
				case 1:_imInfo.ImageStartTypeSize = 1;//Byte
				case 2: DE_val_size = (_IFD.p)[i].size; break;//�ı����ͣ�7λAscii���1λ������0
				case 3:   _imInfo.ImageStartTypeSize = 2; DE_val_size = (_IFD.p)[i].size * sizeof(uint16_t); break;//2�ֽ�
				case 4: _imInfo.ImageStartTypeSize = 4; DE_val_size = (_IFD.p)[i].size * sizeof(uint32_t); break;//4�ֽ�
				case 5:   _imInfo.ImageStartTypeSize = 8; DE_val_size = (_IFD.p)[i].size * sizeof(uint64_t); break;//�ݲ�׼ȷ������������һ�޷���LongΪС�����ڶ���Ϊ��������
				case 6:  _imInfo.ImageStartTypeSize = 1; DE_val_size = (_IFD.p)[i].size * sizeof(int8_t); break;
				case 8:  _imInfo.ImageStartTypeSize = 2; DE_val_size = (_IFD.p)[i].size * sizeof(int16_t); break;
				case 9:   _imInfo.ImageStartTypeSize = 4; DE_val_size = (_IFD.p)[i].size * sizeof(int32_t); break;
				case 10:  _imInfo.ImageStartTypeSize = 8; DE_val_size = (_IFD.p)[i].size * sizeof(int64_t); break;///�ݲ�׼ȷ������������һLongΪС�����ڶ���Ϊ��������
				case 11: _imInfo.ImageStartTypeSize = 4; DE_val_size = (_IFD.p)[i].size * sizeof(float); break;
				case 12:  _imInfo.ImageStartTypeSize = 8; DE_val_size = (_IFD.p)[i].size * sizeof(double); break;
				default: _imInfo.ImageStartTypeSize = 1;  DE_val_size = (_IFD.p)[i].size; break;
				}
				break;// ��ɫ���  ֵ��1Ϊ��ɫ����4Ϊ16ɫ����8Ϊ256ɫ��������������ݸ�����2����˵�������ͼ��
			}
			case 0x0103: break;// ͼ���Ƿ�ѹ�� 05��ʾѹ��
			case 262: {// ��ʾͼ�������: 0-WhiteisZero 1-BlackisZero 2-RGBImg 3-PaletteColor ...
				_imInfo.channel = _IFD.p[i].valOffset == 2 ? 3 : 1;
				break;
			}
			case 0x0111: {// ͼ��������ʼ��ַ�洢λ��������ļ���ʼ��λ��val�ı���λ��
				_imInfo.ImageStart = _IFD.p[i].valOffset;
				break;
			}
			case 274: break;// ͼ������ϵ��ʽ: 1[left top] 2[right top] 3[bottom right] ...
			case 277: break;
			case 278: break;// ��ʾÿһ��Strip�ڰ�����ͼ������� eg:Img[w][h][w] --> RowsPerStrip=1
			case 0x0117: {
				_imInfo.byteNum = _IFD.p[i].valOffset;
				break; // ͼ���ֽ���������ż�����油��
			}
			case 284: break;// ȫ��ͼ��ÿһ�����ص����з�ʽ: 1[RGB...RGB]-���� 2[[R...R],[G...G],[B...B]]-ƽ��
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
		for (uint16_t i = 0; i < _IFD.n; ++i) {//������������һ��IFH���ļ�
			fd.write((char*)( & (_IFD.p[i].tag)),sizeof(_IFD.p->tag));
			fd.write((char*)(&(_IFD.p[i].type)), sizeof(_IFD.p->type));
			fd.write((char*)(&(_IFD.p[i].size)), sizeof(_IFD.p->size));
			fd.write((char*)(&(_IFD.p[i].valOffset)), sizeof(_IFD.p->valOffset));
		}
		fd.write((char*)(&(_IFD.nextIfdOffset)),sizeof(_IFD.nextIfdOffset)); // ��һ��ͼ���ļ������λ�ã�һ����ڵ�һ�ļ�������Ϊ0
		fd.seekp(_imInfo.ImageStart,ios::beg);
		uint64_t byteNum = _imInfo.height * _imInfo.width * _imInfo.channel * _imInfo.ImageStartTypeSize;
		for (uint64_t i = 0; i < byteNum;++i) {//дͼ������
			fd.write((char*)(&(data[i])), sizeof(data[i]));
		}
		fd.close();
	}

}


