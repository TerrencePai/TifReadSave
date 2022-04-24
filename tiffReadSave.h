#pragma once
#pragma once
#include <string>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <vector>
using namespace std;
//#define MC_GET_16(__data__) ((__data__ & 0xFF) << 8) | (__data__ >> 8)
//#define MC_GET_64(__data__) ((__data__ & 0xFF) << 24) | ((__data__ & 0xFF00) << 8) | ((__data__ & 0xFF0000) >> 8) | ((__data__ & 0xFF000000) >> 24)

template<typename T>
struct complex {
	T re;
	T im;
	complex() : re(T{}), im(T{}) {};
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

typedef struct IFDclass
{
	uint16_t n; // ��ʾ��IFD�����˶��ٸ�DE,������ĿΪn
	DE* p; // n��DE
	uint32_t nextIfdOffset; // ��һ��IFD��ƫ����,���û������Ϊ0
	IFDclass() :p(nullptr) {};
	~IFDclass()  {
		free(p);
		p = nullptr;
	};
}IFD;

typedef enum { whiteIsZeros = 0, BlackIsZeros = 1, RGB = 2, RGBPalette = 3, transparencyMask = 4, KCMYK = 5, YCbCr = 6, CIELAB = 8 }PhotometricInterpretation;

typedef struct imgInfo
{
	PhotometricInterpretation  _PhotometricInterpretation;
	uint16_t stripsPerImage;
	uint16_t* BitsPerSample;//
	uint16_t channel;  // ��ʾͼ��ͨ����
	uint16_t orientation;
	uint16_t subFileNum;
	uint32_t height;  // ��ʾͼ��߶�
	uint32_t width; // ��ʾͼ����
	uint32_t* stripOffsets;// ͼ������������ʼ��ַ�洢λ��������ļ���ʼ��λ��val�ı���λ�ã�����stripsPerImage��
	uint32_t* stripByteCouts;
	uint16_t  Compression;//1����û��ѹ����7����JPEGѹ����32773����packBitsѹ��
	uint32_t rowsPerStrip;
	uint32_t allBytes;//ȫ��������
	imgInfo() :_PhotometricInterpretation(BlackIsZeros), stripsPerImage(1), channel(1), allBytes(0),
		orientation(1), subFileNum(1), height(0), width(0), Compression(1), rowsPerStrip(height),
		BitsPerSample(nullptr),stripOffsets(nullptr),stripByteCouts(nullptr){};
	~imgInfo() {
		free(BitsPerSample);
		BitsPerSample = nullptr;
		free(stripOffsets);
		stripOffsets = nullptr;
		free(stripByteCouts);
		stripByteCouts = nullptr;
	}
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

template<typename baseType>
struct tiffTypeMap {
	uint16_t typeValue = 1;
};
template<>
struct tiffTypeMap<uint8_t> {
	uint16_t typeValue = 1;
};
template<>
struct tiffTypeMap<uint16_t> {
	uint16_t typeValue = 3;
};
template<>
struct tiffTypeMap<uint32_t> {
	uint16_t typeValue = 4;
};
template<>
struct tiffTypeMap<uint64_t> {
	uint16_t typeValue = 5;
};
template<>
struct tiffTypeMap<int8_t> {
	uint16_t typeValue = 6;
};
template<>
struct tiffTypeMap<int16_t> {
	uint16_t typeValue = 8;
};
template<>
struct tiffTypeMap<int32_t> {
	uint16_t typeValue = 9;
};
template<>
struct tiffTypeMap<int64_t> {
	uint16_t typeValue = 10;
};
template<>
struct tiffTypeMap<float> {
	uint16_t typeValue = 11;
};
template<>
struct tiffTypeMap<double> {
	uint16_t typeValue = 12;
};


class TiffImg {
public:
	imInfo _imInfo;
	uint8_t* data;
	IFH _IFH;
	IFD _IFD;
	TiffImg(string path);
	template<typename T>
	TiffImg(T, uint32_t height, uint32_t width, uint8_t channel = 1);
	void save(string savePath = "defaultSave.tif");
	~TiffImg() {
		save();
		free(data);
		data = nullptr;
	}
	template<typename T>
	T& operator()(T, uint32_t row, uint32_t col, uint8_t depth = 1) {
		uint64_t sum = ((row - 1) * _imInfo.width + (col - 1)) * chanBytes(_imInfo.channel, _imInfo.BitsPerSample) + chanBytes(depth-1, _imInfo.BitsPerSample);
		return *((T*)(data + sum));
	}
private:
	bool EndianTrans;
	bool read(string path, uint32_t IFDoffset=0);
	const uint16_t littleEndian = 0x4949;
	const uint16_t bigEndian = 0x4d4d;
	uint8_t chanBytes(uint16_t channel, uint16_t* BitsPerSample);
};
uint8_t TiffImg::chanBytes(uint16_t channel, uint16_t* BitsPerSample) {
	uint8_t res{};
	for (int i = 0; i < channel; ++i) {
		res += (BitsPerSample[i] / 8 + BitsPerSample[i] % 8);
	}
	return res;
};
template<typename T>
TiffImg::TiffImg(T, uint32_t height, uint32_t width, uint8_t channel) {
	_imInfo.height = height;
	_imInfo.width = width;
	_imInfo.channel = channel;
	_imInfo.BitsPerSample = sizeof(T)*8;
	_imInfo.stripByteCouts[0] = height * width * channel * sizeof(T);
	if (_imInfo.stripByteCouts[0] % 2 != 0) {
		++_imInfo.stripByteCouts[0];
	}
	uint8_t typeLabel = tiffTypeMap<T>().typeValue;
	//_imInfo.ImageStart[0] = sizeof(_IFH);
	_imInfo.stripOffsets[0] = 22550;
	data = (uint8_t*)malloc(_imInfo.stripByteCouts[0]);
	EndianTrans = false;
	_IFH.endian = mashineEndian ? littleEndian : bigEndian;
	_IFH.magic = 42;
	_IFH.ifdOffset = _imInfo.stripByteCouts[0] + sizeof(_IFH);
	_IFH.ifdOffset = sizeof(_IFH);
	_IFD.n = 6;
	_IFD.p = (DE*)malloc(_IFD.n * sizeof(DE));
	memset((void*)_IFD.p, 0, _IFD.n * sizeof(DE));
	DE* temp = _IFD.p;
	temp->tag = 0x0100; temp->type; temp->size; temp->valOffset = _imInfo.width;
	++temp; temp->tag = 0x0101; temp->type; temp->size; temp->valOffset = _imInfo.height;
	++temp; temp->tag = 0x0102; temp->type = typeLabel; temp->size; temp->valOffset;
	++temp; temp->tag = 0x0106; temp->type; temp->size; temp->valOffset = _imInfo.channel;
	++temp; temp->tag = 0x0111; temp->type; temp->size; temp->valOffset = _imInfo.stripOffsets[0];
	++temp; temp->tag = 0x0117; temp->type; temp->size; temp->valOffset = _imInfo.stripByteCouts[0];

	_IFD.nextIfdOffset = 1;
}

TiffImg::TiffImg(string path) {
	_IFD.nextIfdOffset = 0;
	bool  hasNext = true;
	_imInfo.subFileNum = 0;
	_imInfo.allBytes = 0;
	while (read(path,_IFD.nextIfdOffset)&&hasNext) {
		ifstream frd(path, ios::in | ios::binary);
		if (frd) {//�ļ��򿪳ɹ�
			auto chanBytes = [](auto channel, auto  BitsPerSample)->uint8_t {
				uint8_t res{};
				for (int i = 0; i < channel; ++i) {
					res += (BitsPerSample[i] / 8 + BitsPerSample[i] % 8);
				}
				return res;
			};		
			auto subSum = _imInfo.height * _imInfo.width * chanBytes(_imInfo.channel, _imInfo.BitsPerSample);
			auto subdat= (uint8_t*)malloc(subSum);
			memset(subdat, 0, subSum);
			data = subdat;
			auto tripBytes = [](auto strips, auto  stripByteCouts)->uint32_t {//i֮ǰ�������ֽ�����
				uint32_t res{};
				for (int i = 0; i < strips; ++i) {
					res += stripByteCouts[i];
				}
				return res;
			};
			int tempsum{};
			for (auto k = 0; k < _imInfo.stripsPerImage; ++k) {
				frd.seekg(static_cast<long>(_imInfo.stripOffsets[k]), ios::beg);
				
				for (auto i = 0; i < _imInfo.stripByteCouts[k];++i) {
					if (_imInfo.Compression == 1) {
						frd.read(reinterpret_cast<char*>(subdat+ tempsum), _imInfo.stripByteCouts[k]);
						tempsum += _imInfo.stripByteCouts[k];
						break;
					}else if (_imInfo.Compression == 7) {
						cerr << "JPEGѹ����ûд";
					}
					else if (_imInfo.Compression == 32773) {
						int8_t temp{};
						frd.read(reinterpret_cast<char*>(&temp), 1);
						if (temp == -128) {
						}
						else if (temp < 0) {
							uint8_t nextdata{};
							frd.read(reinterpret_cast<char*>(&nextdata), 1);
							memset(subdat + tempsum, nextdata, 1-temp);
							tempsum += 1 - temp;
							++i;
						}
						else {//�ֿ���������
							frd.read(reinterpret_cast<char*>(subdat + tempsum), temp+1);
							tempsum += 1 + temp;
							i += temp + 1;
						}

					}
				}
				if (EndianTrans) {//��С��ת��,��δ���
					cerr << "��С��ת����ûд";
				}
			
			}
			if (!_IFD.nextIfdOffset) {
				hasNext = false;
			}
			frd.close();
			_imInfo.allBytes +=subSum;
			++_imInfo.subFileNum;
		}
	}
};
uint8_t getsize(uint16_t typeLabel){
	switch (typeLabel) {
	case 1: ;//Byte
	case 2:return 1; break;//�ı����ͣ�7λAscii���1λ������0
	case 3:; return sizeof(uint16_t); break;//2�ֽ�
	case 4:  return sizeof(uint32_t); break;//4�ֽ�
	case 5:;  return sizeof(uint64_t); break;//����������һ�޷���LongΪ���ӣ��ڶ���Ϊ��ĸ����
	case 6:;  return sizeof(int8_t); break;
	case 8:; return sizeof(int16_t); break;
	case 9:;  return sizeof(int32_t); break;
	case 10:;  return sizeof(int64_t); break;///����������һ�޷���LongΪ���ӣ��ڶ���Ϊ��ĸ����
	case 11:; return sizeof(float); break;
	case 12:;  return sizeof(double); break;
	default:;   return 1; break;
	}
}
bool TiffImg::read(string path, uint32_t IfdOffset)
{
	ifstream frd(path, ios::in | ios::binary);
	while(frd) {//�ļ��򿪳ɹ�
		frd.seekg(IfdOffset, ios::beg);
		frd.read(reinterpret_cast<char*>(&(_IFH)), sizeof(_IFH));
		bool imEndian{ _IFH.endian == littleEndian }; //ͼ��洢��ʽ�Ǵ��(false)����С��(true)
		EndianTrans = (imEndian != mashineEndian());
		if (EndianTrans) {
			//_IFH.endian = MC_GET_16(_IFH.endian);
			//_IFH.magic = MC_GET_16(_IFH.magic);
			//_IFH.ifdOffset = MC_GET_64(_IFH.ifdOffset);
		}
		auto IFD_Start = _IFH.ifdOffset;
		frd.seekg(static_cast<long>(IFD_Start), ios::beg);
		frd.read(reinterpret_cast<char*>(&(_IFD.n)), sizeof(_IFD.n));
		if (EndianTrans) {
			;
		}
		auto DE_n = _IFD.n;
		_IFD.p = (DE*)malloc(sizeof(DE) * DE_n);
		frd.read(reinterpret_cast<char*>(_IFD.p), sizeof(DE)* DE_n);

		auto getNdata = [path](auto & DE_ele, auto* res)->void {
			uint8_t typ=getsize(DE_ele.type);
			auto DE_val_size= typ * DE_ele.size;
			if (DE_val_size > 4) {
				ifstream fr(path, ios::in | ios::binary);
				fr.seekg(static_cast<long>(DE_ele.valOffset), ios::beg);
				fr.read(reinterpret_cast<char*>(res), DE_val_size);
				fr.close();
			}
			else {
				memcpy(reinterpret_cast<char*>(res), reinterpret_cast<char*>(&DE_ele.valOffset), DE_val_size);
			}
		};
		
		for (uint32_t i = 0; i < DE_n; ++i) {
			DE & temp = _IFD.p[i];
			switch (temp.tag) {
				
			case 0x00FE: {//�µ����ļ����ͱ�ʶ long��1
				if (temp.valOffset & 1) {
						//����������ͼ
				}
				if (temp.valOffset & 2) {
					//�����ҳͼ���е�ĳһҳ
				}
				if (temp.valOffset & 4) {
					//�����ҳͼ���е�ĳһҳ
				}
				break;
			}
			case 0x00FF: {//���ļ����ͱ�ʶ short��1  ����ʱtag��
				if (temp.valOffset ==1) {
					//������ȫ�ֱ���ͼ��
				}
				if (temp.valOffset== 2) {
					//������С�ֱ���ͼ��
				}
				if (temp.valOffset==3) {
					//�����ҳͼ���е�ĳһҳ
				}
				break;
			}	  
			case 0x0100: getNdata(temp,&_imInfo.width); break;// ͼ���ȴ洢λ��
			case 0x0101: getNdata(temp, &_imInfo.height); break;// ͼ��߶ȴ洢λ��
			case 0x0102: {//ÿ��ͨ����Bit�� SHORT ������ΪSamplesPerPixel
				_imInfo.channel = temp.size;
				_imInfo.BitsPerSample = new uint16_t[_imInfo.channel];
				getNdata(temp, _imInfo.BitsPerSample);
				break;
			}
			case 0x0103: {// ͼ���Ƿ�ѹ�� 05��ʾѹ�� short��1
				getNdata(temp, &_imInfo.Compression);
				break;
			}
			case 0x0106: {// ��ʾͼ�������
				_imInfo._PhotometricInterpretation=(PhotometricInterpretation)_IFD.p[i].valOffset;
				break;
			}
			case 0x0111: {// ͼ������������ʼ��ַ�洢λ��������ļ���ʼ��λ��val�ı���λ��
				_imInfo.stripsPerImage = _IFD.p[i].size;
				_imInfo.stripOffsets = new uint32_t[_imInfo.stripsPerImage];
				getNdata(temp, _imInfo.stripOffsets);
				break;
			}
			case 0x0112: {// 
				getNdata(temp, &_imInfo.orientation);
				break;
			}
			case 0x0114: break;// ͼ������ϵ��ʽ: 1[left top] 2[right top] 3[bottom right] ...
			case 0x0115:getNdata(_IFD.p[i], &_imInfo.channel);  break;//����ͨ���� short ��1
			case 0x0116: {
				getNdata(temp, &_imInfo.rowsPerStrip);
				break;// ��ʾÿһ��Strip�ڰ�����ͼ������� eg:Img[w][h][w] --> RowsPerStrip=1
			}
			case 0x0117: {
				_imInfo.stripsPerImage = _IFD.p[i].size;
				_imInfo.stripByteCouts = new uint32_t[_imInfo.stripsPerImage];
				getNdata(temp, _imInfo.stripByteCouts);
				break; // ͼ���ֽ���������ż�����油��
			}
			case 284: break;// ȫ��ͼ��ÿһ�����ص����з�ʽ: 1[RGB...RGB]-���� 2[[R...R],[G...G],[B...B]]-ƽ��
			case 0x0128:
				break;
			case 0x011A:
				//X�ֱ��ʣ�5��rational
				break;
			case 0x011B:
				break;//Y�ֱ��ʣ�5��rational
			default:  break;
			}
		}
		frd.read(reinterpret_cast<char*>(&_IFD.nextIfdOffset), sizeof(_IFD.nextIfdOffset));
		frd.close();
		return true;
	}
	return false;
}
void TiffImg::save(string savePath) {

	ofstream fd{ savePath, ios::out | ios::binary };
	if (fd.is_open()) {
		fd.seekp(0L, ios::beg);
		if (EndianTrans) {
			uint16_t Endian = (_IFH.endian == littleEndian ? bigEndian : littleEndian);
			fd.write((char*)(&(Endian)), sizeof(_IFH.endian));
		}
		else {
			fd.write((char*)&(_IFH.endian), sizeof(_IFH.endian));
		}
		fd.write((char*)&(_IFH.magic), sizeof(_IFH.magic));
		fd.write((char*)&(_IFH.ifdOffset), sizeof(_IFH.ifdOffset));
		fd.seekp(_IFH.ifdOffset, ios::beg);
		fd.write((char*)&(_IFD.n), sizeof(_IFD.n));
		fd.write((char*)(_IFD.p), sizeof(DE)* _IFD.n);
		fd.write((char*)(&(_IFD.nextIfdOffset)), sizeof(_IFD.nextIfdOffset)); // ��һ��ͼ���ļ������λ�ã�һ����ڵ�һ�ļ�������Ϊ0
		fd.seekp(_imInfo.stripOffsets[0], ios::beg);
		fd.write((char*)(data), _imInfo.allBytes);
		fd.close();
	}

}
