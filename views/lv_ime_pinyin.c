/**
 * @file lv_ime_pinyin.c
 * 从8.3主线拿来的拼音输入法
 * 尝试升级lvgl版本结果出现一堆毛病，于是不升了，稳定最重要
 * 这个输入法也是毛病多多
 * 100ask是怎么在有明显数组越界问题的情况下把这玩意提交到主线里的？？？
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_ime_pinyin.h"
#if LV_USE_IME_PINYIN != 0

#include <stdio.h>

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS    &lv_ime_pinyin_class

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_ime_pinyin_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_ime_pinyin_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void lv_ime_pinyin_style_change_event(lv_event_t * e);
static void lv_ime_pinyin_kb_event(lv_event_t * e);
static void lv_ime_pinyin_cand_panel_event(lv_event_t * e);

static void init_pinyin_dict(lv_obj_t * obj, lv_pinyin_dict_t * dict);
static void pinyin_input_proc(lv_obj_t * obj);
static void pinyin_page_proc(lv_obj_t * obj, uint16_t btn);
static char * pinyin_search_matching(lv_obj_t * obj, char * py_str, uint16_t * cand_num);
static void pinyin_ime_clear_data(lv_obj_t * obj);

#if LV_IME_PINYIN_USE_K9_MODE
    static void pinyin_k9_init_data(lv_obj_t * obj);
    static void pinyin_k9_get_legal_py(lv_obj_t * obj, char * k9_input, const char * py9_map[]);
    static bool pinyin_k9_is_valid_py(lv_obj_t * obj, char * py_str);
    static void pinyin_k9_fill_cand(lv_obj_t * obj);
    static void pinyin_k9_cand_page_proc(lv_obj_t * obj, uint16_t dir);
#endif

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t lv_ime_pinyin_class = {
    .constructor_cb = lv_ime_pinyin_constructor,
    .destructor_cb  = lv_ime_pinyin_destructor,
    .width_def      = LV_SIZE_CONTENT,
    .height_def     = LV_SIZE_CONTENT,
    .group_def      = LV_OBJ_CLASS_GROUP_DEF_TRUE,
    .instance_size  = sizeof(lv_ime_pinyin_t),
    .base_class     = &lv_obj_class
};

#if LV_IME_PINYIN_USE_K9_MODE
static char * lv_btnm_def_pinyin_k9_map[LV_IME_PINYIN_K9_CAND_TEXT_NUM + 21] = {\
                                                                                ",\0", "1#\0",  "abc \0", "def\0",  LV_SYMBOL_BACKSPACE"\0", "\n\0",
                                                                                ".\0", "ghi\0", "jkl\0", "mno\0",  LV_SYMBOL_KEYBOARD"\0", "\n\0",
                                                                                "?\0", "pqrs\0", "tuv\0", "wxyz\0",  LV_SYMBOL_NEW_LINE"\0", "\n\0",
                                                                                LV_SYMBOL_LEFT"\0", "\0"
                                                                               };

static lv_btnmatrix_ctrl_t default_kb_ctrl_k9_map[LV_IME_PINYIN_K9_CAND_TEXT_NUM + 17] = { 1 };
static char   lv_pinyin_k9_cand_str[LV_IME_PINYIN_K9_CAND_TEXT_NUM + 2][LV_IME_PINYIN_K9_MAX_INPUT] = {0};
#endif

static char   lv_pinyin_cand_str[LV_IME_PINYIN_CAND_TEXT_NUM][4];
static char * lv_btnm_def_pinyin_sel_map[LV_IME_PINYIN_CAND_TEXT_NUM + 3];

#if LV_IME_PINYIN_USE_DEFAULT_DICT
lv_pinyin_dict_t lv_ime_pinyin_def_dict[] = {
    {"a", "啊阿"},
    {"ai", "爱哎唉矮挨哀癌碍艾埃隘蔼皑"},
    {"an", "安按暗案岸俺黯鞍氨胺铵桉谙鹌庵埯揞犴"},
    {"ang", "昂肮盎"},
    {"ao", "奥傲熬凹袄澳懊翱敖嗷"},
    {"b", "不被把本吧"},
    {"ba", "把八爸吧巴罢拔霸坝疤靶扒跋芭叭捌笆耙"},
    {"bai", "白百摆败拜伯柏佰稗"},
    {"ban", "办半班般版板搬伴拌扮斑瓣绊扳颁"},
    {"bang", "帮棒榜绑磅邦膀傍镑蚌梆谤"},
    {"bao", "报保包宝抱爆暴薄饱堡炮胞剥鲍豹苞雹褒刨曝"},
    {"bei", "被北背杯备倍悲辈碑贝卑惫钡狈焙"},
    {"ben", "本奔笨苯犇"},
    {"beng", "蹦崩绷泵甭迸蚌"},
    {"bi", "比必笔壁避毕闭币鼻逼臂弊彼秘辟碧蔽鄙陛毙庇痹敝蓖毖"},
    {"bian", "边变便编遍辨辩鞭贬辫扁卞"},
    {"biao", "表标彪镖婊飙膘裱"},
    {"bie", "别憋瘪鳖"},
    {"bin", "滨宾彬斌濒摈"},
    {"bing", "并病兵冰饼丙屏柄秉炳"},
    {"bo", "播博伯波薄拨脖驳泊卜剥玻勃搏菠膊渤钵柏般铂箔帛舶"},
    {"bu", "不部布步补捕簿怖哺埠卜埔"},
    {"c", "从此才次存"},
    {"ca", "擦拆嚓"},
    {"cai", "才菜采彩财材猜裁踩蔡睬"},
    {"can", "参餐残惨灿蚕惭掺"},
    {"cang", "藏仓苍舱沧"},
    {"cao", "草操曹槽糙"},
    {"ce", "测策侧册厕恻"},
    {"cen", "参岑"},
    {"ceng", "曾层蹭"},
    {"cha", "查差插察茶叉刹岔茬诧碴搽"},
    {"chai", "拆柴差豺"},
    {"chan", "产缠铲颤蝉掺搀馋阐谗单"},
    {"chang", "长常场唱厂畅尝偿肠倡昌敞裳猖"},
    {"chao", "超吵朝潮炒抄钞嘲巢绰剿"},
    {"che", "车彻撤扯澈掣"},
    {"chen", "陈沉晨臣趁尘衬辰忱郴宸琛嗔谶"},
    {"cheng", "成程城称承乘诚盛橙撑惩呈澄骋逞秤"},
    {"chi", "吃尺持迟池赤齿翅斥痴耻驰匙炽弛侈"},
    {"chong", "重冲充虫宠崇"},
    {"chou", "抽愁丑臭筹仇酬瞅绸稠踌畴"},
    {"chu", "出处除初触楚础储厨锄雏畜橱矗搐滁躇"},
    {"chuai", "揣踹搋"},
    {"chuan", "传船穿串川喘椽"},
    {"chuang", "创窗床闯疮"},
    {"chui", "吹垂锤捶炊椎"},
    {"chun", "春纯唇蠢醇淳椿"},
    {"chuo", "戳绰龊啜辍绰"},
    {"ci", "次此词刺辞慈磁赐瓷雌伺兹疵差茨"},
    {"cong", "从聪丛匆葱囱"},
    {"cou", "凑"},
    {"cu", "粗促醋簇卒"},
    {"cuan", "窜攒篡蹿"},
    {"cui", "催翠脆摧崔粹淬瘁"},
    {"cun", "村存寸"},
    {"cuo", "错措搓挫磋撮"},
    {"d", "的地得都多"},
    {"da", "大打达答搭哒沓妲耷"},
    {"dai", "带代待戴袋呆逮贷歹殆怠傣"},
    {"dan", "但单蛋担淡弹胆旦诞氮耽惮掸石丹郸"},
    {"dang", "当党档挡荡"},
    {"dao", "到道倒导刀岛稻盗悼蹈捣祷"},
    {"de", "的得地德の"},
    {"dei", "得嘚"},
    {"deng", "等灯登邓凳瞪蹬澄"},
    {"di", "地的第低弟帝递底敌滴抵堤迪蒂缔嫡涤笛狄翟提"},
    {"dian", "电点店典垫殿甸淀掂碘颠惦奠佃靛滇"},
    {"diao", "调掉吊钓雕叼刁凋碉"},
    {"die", "跌叠爹蝶碟迭谍"},
    {"ding", "定丁顶订钉盯叮鼎锭"},
    {"diu", "丢"},
    {"dong", "动东懂冬洞冻栋董侗恫"},
    {"dou", "都豆斗抖逗陡兜痘"},
    {"du", "都度读独毒肚渡赌堵杜镀睹妒督犊"},
    {"duan", "段短断端锻缎"},
    {"dui", "对队堆兑"},
    {"dun", "顿蹲盾吨敦钝墩囤遁"},
    {"duo", "多朵躲夺度舵堕剁跺垛惰哆掇"},
    {"e", "恶额饿鹅俄阿扼蛾鄂娥厄讹遏峨"},
    {"en", "嗯恩摁蒽"},
    {"er", "儿而二耳尔贰洱饵"},
    {"f", "发分方法放"},
    {"fa", "发法罚阀乏伐筏珐"},
    {"fan", "反饭烦翻范凡犯繁泛贩返帆番樊矾钒蕃犯"},
    {"fang", "方放房防访仿纺芳坊妨肪"},
    {"fei", "非飞费废肺肥匪沸啡菲诽吠"},
    {"fen", "分粉份奋愤纷氛坟焚芬酚吩忿汾粪"},
    {"feng", "风封丰蜂峰锋疯缝逢奉讽冯凤烽枫"},
    {"fo", "佛"},
    {"fou", "否缶"},
    {"fu", "服复付副负富父福府妇扶浮符幅伏附腐腹咐敷肤辅抚赴覆赋傅俘拂辐弗俯甫脯斧腑釜阜袱讣佛涪"},
    {"g", "个工高过更"},
    {"ga", "嘎咖夹噶"},
    {"gai", "该改盖概钙溉芥"},
    {"gan", "感干敢赶肝杆甘乾赣竿秆柑"},
    {"gang", "刚钢港岗纲杠缸扛冈肛"},
    {"gao", "高告搞稿糕膏羔镐皋篙"},
    {"ge", "个各歌格哥割隔革阁胳搁鸽葛蛤咯戈铬疙"},
    {"gei", "给"},
    {"gen", "跟根亘哏艮"},
    {"geng", "更耕梗庚颈耿羹埂粳"},
    {"gong", "工公共供功宫恭贡拱巩弓躬汞龚"},
    {"gou", "够狗购构勾沟钩苟垢"},
    {"gu", "古故顾骨鼓股谷姑固雇孤估咕菇沽辜箍蛊贾"},
    {"gua", "挂瓜刮寡褂剐"},
    {"guai", "怪拐乖"},
    {"guan", "关观管官馆惯冠灌贯罐纶棺"},
    {"guang", "光广逛咣胱"},
    {"gui", "贵归鬼规柜龟硅桂跪轨闺瑰诡癸傀刽圭炔"},
    {"gun", "滚棍辊"},
    {"guo", "国过果锅郭裹涡"},
    {"g", "好会和还后"},
    {"ha", "哈蛤"},
    {"hai", "还海孩害咳骇亥氦骸"},
    {"han", "含汉寒韩喊汗函旱憾涵罕翰撼捍憨酣焊邯"},
    {"hang", "行航杭巷夯吭"},
    {"hao", "好号豪浩耗毫郝壕嚎镐貉"},
    {"he", "和合何河喝核贺荷盒呵吓褐鹤赫禾菏阂涸貉"},
    {"hei", "黑嘿"},
    {"hen", "很恨狠痕"},
    {"heng", "行横恒衡哼亨"},
    {"hong", "红宏洪轰虹鸿哄弘烘"},
    {"hou", "后候厚猴吼侯喉"},
    {"hu", "和护湖虎户互胡呼乎糊壶狐沪忽唬葫蝴瑚弧"},
    {"hua", "化花话画华划滑哗猾"},
    {"huai", "坏怀淮槐徊"},
    {"huan", "还换欢环缓幻患唤焕涣桓宦痪豢"},
    {"huang", "黄荒皇慌晃谎惶煌恍凰蝗磺簧幌"},
    {"hui", "会回灰汇挥绘惠辉悔毁慧徽恢讳诲贿晦秽卉烩蛔"},
    {"hun", "混婚魂浑昏荤"},
    {"huo", "火活或和获货伙霍祸惑豁"},
    {"i", ""},
    {"ji", "机己及即计记系济级积极集激急继际纪基击鸡技寄既季迹挤肌籍疾吉忌剂祭冀稽给畸嫉汲脊伎悸妓蓟缉藉齐奇期"},
    {"jia", "家加价假架甲驾佳嫁夹嘉钾贾颊荚枷稼茄缴"},
    {"jian", "见间件建简坚检减健键渐荐剑鉴兼肩监尖煎剪捡拣舰箭践贱溅俭艰奸笺茧柬碱缄槛涧饯浅"},
    {"jiang", "江将讲降奖姜酱蒋强匠疆浆桨僵"},
    {"jiao", "叫交教觉角较脚焦胶娇骄饺郊浇搅绞校轿狡矫酵缴剿蕉椒礁铰嚼侥窖"},
    {"jie", "结解街接节界借阶姐介截杰揭戒届洁捷竭诫皆桔睫芥藉秸疥"},
    {"jin", "进金今近尽紧仅斤禁津锦劲谨巾筋晋浸襟靳烬"},
    {"jing", "经精京竟井景境静净警镜敬睛颈竞径晶鲸荆劲靖茎惊痉兢粳"},
    {"jiong", "囧窘炯"},
    {"jiu", "就九久旧救酒究纠舅揪臼灸疚咎韭玖厩"},
    {"ju", "举局据具距句居巨剧聚拒俱惧锯菊拘矩狙沮桔鞠踞炬车驹咀疽"},
    {"juan", "卷圈捐娟眷倦绢鹃"},
    {"jue", "觉绝决角掘诀爵倔嚼抉撅攫"},
    {"jun", "军均君俊菌郡竣峻骏钧浚"},
    {"ka", "卡咖咔喀"},
    {"kai", "开凯楷慨揩"},
    {"kan", "看砍刊坎勘堪侃槛瞰龛"},
    {"kang", "抗康扛炕慷亢糠"},
    {"kao", "靠考烤拷铐犒尻"},
    {"ke", "可课客克科刻渴颗壳咳棵柯磕苛坷嗑"},
    {"ken", "肯恳垦啃"},
    {"keng", "坑吭铿"},
    {"kong", "空控孔恐箜"},
    {"kou", "口扣抠叩寇蔻"},
    {"ku", "苦库哭酷裤枯窟"},
    {"kua", "夸跨垮挎胯"},
    {"kuai", "快块会筷侩脍"},
    {"kuan", "宽款髋"},
    {"kuang", "况狂矿框旷眶筐匡哐诓"},
    {"kui", "亏窥魁溃愧葵盔馈奎傀岿"},
    {"kun", "困昆捆坤"},
    {"kuo", "扩阔括廓"},
    {"l", "了来里两来"},
    {"la", "拉啦辣落腊蜡喇垃"},
    {"lai", "来赖莱"},
    {"lan", "蓝兰烂懒栏览揽篮拦缆滥澜阑谰婪"},
    {"lang", "浪郎狼朗廊榔琅"},
    {"lao", "老劳落牢捞姥佬烙酪涝潦"},
    {"le", "了乐勒肋"},
    {"lei", "累类雷泪垒勒肋蕾擂磊镭儡"},
    {"leng", "冷棱楞"},
    {"li", "力里理利立离李例丽历礼粒莉栗励隶璃厘黎梨狸篱鲤砾沥荔哩痢俐吏厉"},
    {"lian", "连联脸练恋炼链怜廉莲帘敛镰涟"},
    {"liang", "两量亮良凉梁粮辆谅俩晾粱"},
    {"liao", "了料聊疗辽廖撩僚镣燎寥撂潦"},
    {"lie", "列烈裂猎劣"},
    {"lin", "林临邻淋琳霖磷鳞凛吝拎赁"},
    {"ling", "领令灵另零岭玲铃凌龄陵伶怜棱羚菱"},
    {"liu", "六流刘留柳陆溜瘤硫榴琉馏"},
    {"long", "龙弄隆笼聋拢垄陇窿咙"},
    {"lou", "楼漏露搂娄篓陋"},
    {"lu", "路录陆露六鹿炉芦鲁绿卢卤禄虏潞麓赂碌戮颅庐掳吕"},
    {"lv", "绿律旅率虑屡滤铝侣履缕驴氯"},
    {"lve", "略掠"},
    {"luan", "乱卵孪峦挛滦"},
    {"lun", "论轮伦沦抡仑纶"},
    {"luo", "落罗络洛螺萝裸骆烙逻锣咯骡箩"},
    {"m", "么没们民名"},
    {"ma", "吗妈马麻嘛码骂玛蚂摩抹么"},
    {"mai", "买卖麦埋迈脉"},
    {"man", "满慢漫蛮埋曼瞒蔓馒谩"},
    {"mang", "忙盲芒茫莽氓"},
    {"mao", "毛猫冒帽贸貌茂茅矛锚铆卯"},
    {"me", "么"},
    {"mei", "没每美妹眉梅媒煤霉媚镁枚玫酶昧寐糜"},
    {"men", "门们闷焖"},
    {"meng", "梦猛盟萌蒙孟锰檬"},
    {"mi", "米密迷秘蜜弥谜觅幂眯糜靡醚泌"},
    {"mian", "面免棉眠绵勉缅冕娩"},
    {"miao", "秒妙苗描庙瞄喵渺藐"},
    {"mie", "灭蔑"},
    {"min", "民敏闽抿皿悯"},
    {"ming", "名明命鸣铭螟"},
    {"miu", "谬缪"},
    {"mo", "摸模末莫默魔磨摩膜墨抹没沫陌漠寞脉摹蘑"},
    {"mou", "某谋牟"},
    {"mu", "目木母亩幕牧慕穆模墓姆募拇牡暮睦姥牟"},
    {"n", "你能那呢年"},
    {"na", "那哪拿纳娜呐钠"},
    {"nai", "乃奶耐奈哪氖"},
    {"nan", "难男南"},
    {"nang", "囊馕囔攮"},
    {"nao", "闹脑恼挠孬瑙呶猱铙淖"},
    {"ne", "呢讷"},
    {"nei", "内那哪馁"},
    {"nen", "嫩恁"},
    {"neng", "能"},
    {"ni", "你泥呢拟腻尼溺逆妮倪匿霓"},
    {"nian", "年念拈碾捻撵粘蔫"},
    {"niang", "娘酿"},
    {"niao", "鸟尿袅脲"},
    {"nie", "捏聂孽涅镍镊啮"},
    {"nin", "您"},
    {"ning", "宁凝拧柠狞泞"},
    {"niu", "牛扭纽钮"},
    {"nong", "农弄浓脓"},
    {"nu", "努怒奴"},
    {"nv", "女钕"},
    {"nue", "虐疟"},
    {"nuan", "暖"},
    {"nuo", "诺挪糯懦娜"},
    {"o", "哦喔噢"},
    {"ou", "偶欧呕区藕殴鸥沤"},
    {"pa", "怕爬啪趴帕扒耙琶"},
    {"pai", "排派拍牌迫徘湃"},
    {"pan", "盘判盼潘攀畔叛胖般番磐"},
    {"pang", "旁胖庞磅膀乓耪"},
    {"pao", "跑炮泡抛袍刨咆"},
    {"pei", "配陪培赔佩坏裴沛呸胚"},
    {"pen", "喷盆"},
    {"peng", "碰朋鹏捧彭蓬膨篷棚砰烹抨硼澎"},
    {"pi", "批皮屁披辟匹疲脾劈啤坯否坏痞僻譬霹毗琵砒"},
    {"pian", "片便篇偏骗扁"},
    {"piao", "票漂飘瓢朴"},
    {"pie", "撇瞥"},
    {"pin", "品频贫拼聘"},
    {"ping", "平评瓶屏凭苹萍坪乒"},
    {"po", "破婆迫坡泼颇泊魄朴粕"},
    {"pou", "剖"},
    {"pu", "普扑铺谱浦朴脯曝瀑仆蒲葡圃莆菩埔堡"},
    {"qi", "其起气七期器企齐奇骑妻弃旗汽泣欺戚漆启契棋岂祈歧栖崎沏脐凄柒砌讫祁迄畦"},
    {"qia", "恰卡洽掐"},
    {"qian", "前千钱签欠浅潜牵迁铅歉纤谦乾嵌遣钳黔谴堑仟扦钎"},
    {"qiang", "强枪墙抢腔呛羌蔷"},
    {"qiao", "巧桥悄敲壳瞧俏翘窍乔侨峭鞘撬橇锹"},
    {"qie", "切且茄窃怯"},
    {"qin", "亲勤琴秦侵禽寝芹钦擒沁"},
    {"qing", "情清请青庆轻倾晴氢亲卿擎顷氰"},
    {"qiong", "穷琼穹"},
    {"qiu", "球求秋丘邱囚酋泅"},
    {"qu", "去区取曲趣驱屈渠娶趋躯龋蛆"},
    {"quan", "全权圈劝拳券犬泉卷醛痊颧"},
    {"que", "确却缺雀鹊瘸榷炔"},
    {"qun", "群裙逡"},
    {"r", "人日然让任"},
    {"ran", "然染燃冉"},
    {"rang", "让嚷壤攘瓤"},
    {"rao", "绕扰饶"},
    {"re", "热惹"},
    {"ren", "人认任忍仁韧刃妊纫壬亻"},
    {"reng", "扔仍"},
    {"ri", "日"},
    {"rong", "容荣绒融溶熔蓉茸戎冗"},
    {"rou", "肉柔揉"},
    {"ru", "如入乳儒汝辱蠕茹褥孺"},
    {"ruan", "软阮"},
    {"rui", "瑞锐睿蕊芮枘"},
    {"run", "闰润"},
    {"ruo", "弱若偌叒"},
    {"s", "是时说三四什似"},
    {"sa", "撒洒萨仨"},
    {"sai", "赛塞腮鳃"},
    {"san", "三散伞叁"},
    {"sang", "桑嗓丧"},
    {"sao", "扫骚嫂梢臊搔缫瘙"},
    {"se", "色塞涩瑟啬铯"},
    {"sen", "森"},
    {"seng", "僧"},
    {"sha", "杀沙傻啥纱砂厦刹杉煞莎"},
    {"shai", "晒色筛"},
    {"shan", "山闪善扇删陕单衫珊擅煽赡栅汕缮杉膳姗苫讪疝鳝膻"},
    {"shang", "上商尚伤赏晌裳汤墒殇觞熵"},
    {"shao", "少烧稍勺绍哨梢邵韶召鞘芍捎"},
    {"she", "设社射舌涉蛇摄舍折赦拾奢赊慑"},
    {"shei", "谁"},
    {"shen", "什深身神甚审沈伸申慎参婶绅渗肾呻娠砷"},
    {"sheng", "生声省胜升圣剩盛乘绳牲甥"},
    {"shi", "是时事使实十试世识市师室施石失势食视士始适诗示释湿史氏似什驶逝誓饰嗜侍仕蚀矢拭匙噬屎柿虱狮峙殖嘘"},
    {"shou", "手收受首守熟售授兽寿瘦"},
    {"shu", "数书术属树熟输述束叔舒殊疏鼠署暑竖恕蜀蔬漱梳庶黍抒赎戍曙孰枢薯墅淑"},
    {"shua", "刷耍"},
    {"shuai", "率帅甩摔衰蟀"},
    {"shuan", "栓拴涮闩"},
    {"shuang", "双爽霜孀"},
    {"shui", "水睡谁说税氵"},
    {"shun", "顺瞬舜吮"},
    {"shuo", "说数硕烁朔铄槊蒴"},
    {"si", "四死思似斯司私丝寺食撕肆伺饲嗣嘶巳"},
    {"song", "送宋松颂诵耸怂讼嵩淞"},
    {"sou", "搜艘嗽嗖馊叟擞飕薮"},
    {"su", "素速诉苏俗宿肃塑溯粟夙酥簌愫僳稣窣"},
    {"suan", "算酸蒜"},
    {"sui", "随岁虽碎遂穗隧隋髓绥尿祟"},
    {"sun", "孙损笋隼榫狲"},
    {"suo", "所缩锁索唆梭琐莎嗦蓑娑羧惢"},
    {"ta", "他她它踏塔塌榻拓蹋沓獭挞铊祂"},
    {"tai", "太台态抬泰胎汰苔钛肽跆酞薹呔"},
    {"tan", "谈探弹摊坦贪滩叹炭碳痰谭潭毯坛檀袒坍瘫"},
    {"tang", "唐糖汤躺堂烫塘趟倘膛淌搪棠敞"},
    {"tao", "套讨淘桃逃陶涛掏萄滔韬绦"},
    {"te", "特忒忑"},
    {"teng", "疼腾藤滕誊"},
    {"ti", "提体替题踢梯剃惕涕蹄啼剔屉嚏锑"},
    {"tian", "天甜田填添舔恬腆钿殄兲"},
    {"tiao", "条跳调挑眺迢窕"},
    {"tie", "铁贴帖餮"},
    {"ting", "听停挺厅庭亭廷艇婷町汀烃霆"},
    {"tong", "同通统痛童桶铜筒桐瞳彤捅侗恫酮"},
    {"tou", "头投透偷骰"},
    {"tu", "图土突途徒涂吐兔凸屠秃余荼"},
    {"tuan", "团湍抟"},
    {"tui", "推退腿褪颓蜕"},
    {"tun", "吞屯臀囤豚鲀"},
    {"tuo", "托脱拖拓妥驼唾椭陀驮鸵坨沱砣"},
    {"u", ""},
    {"v", ""},
    {"w", "我为无五万完玩"},
    {"wa", "哇瓦娃挖袜蛙洼凹佤娲"},
    {"wai", "外歪崴"},
    {"wan", "万完玩晚湾碗挽腕弯顽婉宛丸蔓皖豌惋蜿烷"},
    {"wang", "王往望忘网亡汪旺枉妄罔惘魍"},
    {"wei", "为位未围委伟卫味微危威唯谓维违尾喂胃畏魏慰伪惟尉纬蔚渭萎苇韦潍巍桅"},
    {"wen", "文问温闻稳纹吻蚊瘟紊雯汶"},
    {"weng", "嗡翁瓮"},
    {"wo", "我握窝卧沃涡蜗喔斡挝"},
    {"wu", "无五物务武误舞屋午吴污雾悟乌吾勿伍诬捂呜巫恶戊钨梧毋坞晤芜"},
    {"x", "下小学想现"},
    {"xi", "系西希习细喜吸息洗戏席稀惜袭析昔悉夕熙栖牺晰犀烯硒矽铣曦汐檄茜"},
    {"xia", "下夏吓虾狭峡侠霞瞎厦辖暇匣"},
    {"xian", "先现显线县险献鲜限陷咸闲仙嫌弦纤贤馅腺宪掀羡衔涎舷锨铣"},
    {"xiang", "想相向象香响乡像项详享祥箱翔橡湘降巷襄镶厢"},
    {"xiao", "小笑校消效晓销萧孝削肖宵啸硝霄哮嚣淆"},
    {"xie", "写谢些血解鞋斜协携卸邪泄泻歇屑谐械胁挟懈蟹楔蝎"},
    {"xin", "心新信欣辛薪芯锌衅忻"},
    {"xing", "行性星形兴醒型姓幸省刑杏邢腥猩惺"},
    {"xiong", "雄熊胸兄凶汹匈"},
    {"xiu", "休修秀袖绣锈羞嗅臭宿朽"},
    {"xu", "需许续须序徐虚绪畜蓄吁絮嘘叙旭戌恤婿墟酗"},
    {"xuan", "选宣玄旋悬轩喧绚眩癣"},
    {"xue", "学血雪穴削靴薛"},
    {"xun", "讯寻训迅询巡循旬熏逊勋殉驯汛浚"},
    {"y", "一有也要与"},
    {"ya", "亚呀牙压雅鸭丫哑押芽崖涯讶衙轧鸦蚜"},
    {"yan", "眼烟言严研演验盐延颜沿燕厌炎宴掩咽衍淹艳雁砚谚彦焰阎焉奄堰蜒阉唁"},
    {"yang", "样杨阳养洋扬羊央氧仰秧漾殃佯鸯疡"},
    {"yao", "要药摇咬邀耀约遥腰谣姚钥妖窑尧瑶舀侥"},
    {"ye", "也业夜爷叶野页液咽耶冶椰掖曳腋噎"},
    {"yi", "一以已义意议依医易衣亿益艺移乙宜异亦遗仪疑忆役译疫翼毅逸椅姨抑倚谊溢伊矣揖沂裔颐胰屹夷臆翌绎彝铱肄"},
    {"yin", "因音印银引饮阴隐迎吟姻尹殷寅淫茵荫"},
    {"ying", "应影英赢营硬映婴樱盈蝇鹰颖莹荧萤缨"},
    {"yo", "哟唷"},
    {"yong", "用永拥涌勇泳庸佣咏雍踊痈臃蛹恿"},
    {"you", "有又由优油游友右尤邮忧犹悠诱幽佑铀釉酉"},
    {"yu", "于与语鱼育遇欲预余玉雨宇羽域娱渔愈狱御郁寓誉裕浴屿予俞逾愉愚喻虞舆迂芋禹瑜峪驭尉盂渝隅羽"},
    {"yuan", "员原元远园愿院圆源缘袁怨援渊冤猿苑辕垣鸳"},
    {"yue", "月约越阅乐悦跃岳粤钥曰"},
    {"yun", "云运均晕允匀蕴韵孕陨酝耘郧"},
    {"z", "在中这只总"},
    {"za", "咋杂砸扎咱匝"},
    {"zai", "在再载灾宰仔栽哉"},
    {"zan", "咱赞暂攒"},
    {"zang", "脏藏葬赃"},
    {"zao", "早造遭糟燥躁噪澡皂枣灶凿藻蚤"},
    {"ze", "则责择泽啧仄"},
    {"zei", "贼"},
    {"zen", "怎"},
    {"zeng", "曾增赠综憎"},
    {"zha", "扎炸诈查眨咋闸榨渣乍栅札轧喳铡柞"},
    {"zhai", "摘宅窄债寨择祭斋翟侧"},
    {"zhan", "战站占展斩粘沾栈颤瞻崭绽盏湛毡詹蘸辗"},
    {"zhang", "长张章掌帐账涨障丈仗胀杖彰漳樟瘴"},
    {"zhao", "找着照招朝兆赵召罩爪沼昭肇"},
    {"zhe", "这者着折浙哲遮辙蔗蛰锗"},
    {"zhen", "真阵镇针振震侦诊贞珍枕斟帧甄臻疹砧"},
    {"zheng", "正证整争政征挣睁郑症蒸拯怔狰"},
    {"zhi", "只之知直指至制志值职质治置止纸支芝植执识织枝汁侄致址趾旨智帜秩滞炙掷挚窒稚痔蜘吱殖峙脂肢"},
    {"zhong", "中重种众终钟忠仲肿衷盅"},
    {"zhou", "周州洲舟皱昼咒宙轴骤肘帚粥诌"},
    {"zhu", "主住注助珠朱猪逐竹祝诸筑属著驻铸嘱烛煮柱贮株蛛蛀诛瞩拄"},
    {"zhua", "抓爪"},
    {"zhuai", "拽"},
    {"zhuan", "转专传赚砖撰篆"},
    {"zhuang", "装壮状庄撞妆桩幢"},
    {"zhui", "追坠锥缀椎赘"},
    {"zhun", "准谆"},
    {"zhuo", "桌着捉卓浊拙啄琢灼茁酌"},
    {"zi", "自子字资仔姿紫滋咨兹孜籽渍滓淄"},
    {"zong", "总宗综纵踪棕鬃"},
    {"zou", "走邹奏揍"},
    {"zu", "组族足租阻祖卒诅"},
    {"zuan", "钻纂攥"},
    {"zui", "最嘴醉罪"},
    {"zun", "尊遵樽鳟"},
    {"zuo", "做作坐座左昨佐琢撮柞"},
    {NULL, NULL}
};
#endif

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
lv_obj_t * lv_ime_pinyin_create(lv_obj_t * parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

/*=====================
 * Setter functions
 *====================*/

/**
 * Set the keyboard of Pinyin input method.
 * @param obj  pointer to a Pinyin input method object
 * @param dict pointer to a Pinyin input method keyboard
 */
void lv_ime_pinyin_set_keyboard(lv_obj_t * obj, lv_obj_t * kb)
{
    if(kb) {
        LV_ASSERT_OBJ(kb, &lv_keyboard_class);
    }

    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    pinyin_ime->kb = kb;
    lv_obj_add_event_cb(pinyin_ime->kb, lv_ime_pinyin_kb_event, LV_EVENT_VALUE_CHANGED, obj);
    lv_obj_align_to(pinyin_ime->cand_panel, pinyin_ime->kb, LV_ALIGN_OUT_TOP_MID, 0, 0);
}

/**
 * Set the dictionary of Pinyin input method.
 * @param obj  pointer to a Pinyin input method object
 * @param dict pointer to a Pinyin input method dictionary
 */
void lv_ime_pinyin_set_dict(lv_obj_t * obj, lv_pinyin_dict_t * dict)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    init_pinyin_dict(obj, dict);
}

/**
 * Set mode, 26-key input(k26) or 9-key input(k9).
 * @param obj  pointer to a Pinyin input method object
 * @param mode   the mode from 'lv_keyboard_mode_t'
 */
void lv_ime_pinyin_set_mode(lv_obj_t * obj, lv_ime_pinyin_mode_t mode)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    LV_ASSERT_OBJ(pinyin_ime->kb, &lv_keyboard_class);

    pinyin_ime->mode = mode;

#if LV_IME_PINYIN_USE_K9_MODE
    if(pinyin_ime->mode == LV_IME_PINYIN_MODE_K9) {
        pinyin_k9_init_data(obj);
        lv_keyboard_set_map(pinyin_ime->kb, LV_KEYBOARD_MODE_USER_1, (const char **)lv_btnm_def_pinyin_k9_map,
                            (const lv_btnmatrix_ctrl_t *)default_kb_ctrl_k9_map);
        lv_keyboard_set_mode(pinyin_ime->kb, LV_KEYBOARD_MODE_USER_1);
    }
#endif
}

/*=====================
 * Getter functions
 *====================*/

/**
 * Set the dictionary of Pinyin input method.
 * @param obj  pointer to a Pinyin IME object
 * @return     pointer to the Pinyin IME keyboard
 */
lv_obj_t * lv_ime_pinyin_get_kb(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    return pinyin_ime->kb;
}

/**
 * Set the dictionary of Pinyin input method.
 * @param obj  pointer to a Pinyin input method object
 * @return     pointer to the Pinyin input method candidate panel
 */
lv_obj_t * lv_ime_pinyin_get_cand_panel(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    return pinyin_ime->cand_panel;
}

/**
 * Set the dictionary of Pinyin input method.
 * @param obj  pointer to a Pinyin input method object
 * @return     pointer to the Pinyin input method dictionary
 */
lv_pinyin_dict_t * lv_ime_pinyin_get_dict(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    return pinyin_ime->dict;
}

/*=====================
 * Other functions
 *====================*/

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_ime_pinyin_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    uint16_t py_str_i = 0;
    uint16_t btnm_i = 0;
    for(btnm_i = 0; btnm_i < (LV_IME_PINYIN_CAND_TEXT_NUM + 3); btnm_i++) {
        if(btnm_i == 0) {
            lv_btnm_def_pinyin_sel_map[btnm_i] = "<";
        }
        else if(btnm_i == (LV_IME_PINYIN_CAND_TEXT_NUM + 1)) {
            lv_btnm_def_pinyin_sel_map[btnm_i] = ">";
        }
        else if(btnm_i == (LV_IME_PINYIN_CAND_TEXT_NUM + 2)) {
            lv_btnm_def_pinyin_sel_map[btnm_i] = "";
        }
        else {
            lv_pinyin_cand_str[py_str_i][0] = ' ';
            lv_btnm_def_pinyin_sel_map[btnm_i] = lv_pinyin_cand_str[py_str_i];
            py_str_i++;
        }
    }

    pinyin_ime->mode = LV_IME_PINYIN_MODE_K26;
    pinyin_ime->py_page = 0;
    pinyin_ime->ta_count = 0;
    pinyin_ime->cand_num = 0;
    lv_memset_00(pinyin_ime->input_char, sizeof(pinyin_ime->input_char));
    lv_memset_00(pinyin_ime->py_num, sizeof(pinyin_ime->py_num));
    lv_memset_00(pinyin_ime->py_pos, sizeof(pinyin_ime->py_pos));

    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);

    lv_obj_set_size(obj, LV_PCT(100), LV_PCT(55));
    lv_obj_align(obj, LV_ALIGN_BOTTOM_MID, 0, 0);

#if LV_IME_PINYIN_USE_DEFAULT_DICT
    init_pinyin_dict(obj, lv_ime_pinyin_def_dict);
#endif

    /* Init pinyin_ime->cand_panel */
    
    pinyin_ime->cand_panel = lv_btnmatrix_create(obj);
    pinyin_ime->kb = lv_keyboard_create(obj);
    
    lv_obj_set_size(pinyin_ime->kb, LV_PCT(100), LV_PCT(85));
    lv_obj_set_size(pinyin_ime->cand_panel, LV_PCT(100), LV_PCT(15));

    lv_obj_align_to(pinyin_ime->cand_panel, pinyin_ime->kb, LV_ALIGN_OUT_TOP_MID, 0, 0);
    lv_btnmatrix_set_map(pinyin_ime->cand_panel, (const char **)lv_btnm_def_pinyin_sel_map);
    lv_obj_add_flag(pinyin_ime->cand_panel, LV_OBJ_FLAG_HIDDEN);

    lv_btnmatrix_set_one_checked(pinyin_ime->cand_panel, true);
    lv_obj_clear_flag(pinyin_ime->cand_panel, LV_OBJ_FLAG_CLICK_FOCUSABLE);

    /* Set cand_panel style*/
    // Default style
    lv_obj_set_style_bg_opa(pinyin_ime->cand_panel, LV_OPA_0, 0);
    lv_obj_set_style_border_width(pinyin_ime->cand_panel, 0, 0);
    lv_obj_set_style_pad_all(pinyin_ime->cand_panel, 8, 0);
    lv_obj_set_style_pad_gap(pinyin_ime->cand_panel, 0, 0);
    lv_obj_set_style_radius(pinyin_ime->cand_panel, 0, 0);
    lv_obj_set_style_pad_gap(pinyin_ime->cand_panel, 0, 0);
    lv_obj_set_style_base_dir(pinyin_ime->cand_panel, LV_BASE_DIR_LTR, 0);

    // LV_PART_ITEMS style
    lv_obj_set_style_radius(pinyin_ime->cand_panel, 12, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(pinyin_ime->cand_panel, lv_color_white(), LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(pinyin_ime->cand_panel, LV_OPA_0, LV_PART_ITEMS);
    lv_obj_set_style_shadow_opa(pinyin_ime->cand_panel, LV_OPA_0, LV_PART_ITEMS);

    // LV_PART_ITEMS | LV_STATE_PRESSED style
    lv_obj_set_style_bg_opa(pinyin_ime->cand_panel, LV_OPA_COVER, LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(pinyin_ime->cand_panel, lv_color_white(), LV_PART_ITEMS | LV_STATE_PRESSED);

    /* event handler */
    lv_obj_add_event_cb(pinyin_ime->cand_panel, lv_ime_pinyin_cand_panel_event, LV_EVENT_VALUE_CHANGED, obj);
    lv_obj_add_event_cb(obj, lv_ime_pinyin_style_change_event, LV_EVENT_STYLE_CHANGED, NULL);
    lv_obj_add_event_cb(pinyin_ime->kb, lv_ime_pinyin_kb_event, LV_EVENT_VALUE_CHANGED, obj);

#if LV_IME_PINYIN_USE_K9_MODE
    pinyin_ime->k9_input_str_len = 0;
    pinyin_ime->k9_py_ll_pos = 0;
    pinyin_ime->k9_legal_py_count = 0;
    lv_memset_00(pinyin_ime->k9_input_str, LV_IME_PINYIN_K9_MAX_INPUT);

    pinyin_k9_init_data(obj);

    _lv_ll_init(&(pinyin_ime->k9_legal_py_ll), sizeof(ime_pinyin_k9_py_str_t));
#endif
}

static void lv_ime_pinyin_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);

    /*
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;
    if(lv_obj_is_valid(pinyin_ime->kb))
        lv_obj_del(pinyin_ime->kb);

    if(lv_obj_is_valid(pinyin_ime->cand_panel))
        lv_obj_del(pinyin_ime->cand_panel);
    */
}

/**
 * 键盘按键事件回调（输入法核心入口）。
 * 用户在软键盘上每按下一个按键都会触发一次 LV_EVENT_VALUE_CHANGED，
 * 本函数根据按键文本 txt 分发到不同的处理分支：
 *   - 回车 / 换行：清空输入数据
 *   - 退格：删除输入缓冲区最后一个字符并重新检索候选字
 *   - 字母键（K26 全键盘）：追加到拼音串并触发候选字检索
 *   - 字母键（K9 九宫格）：按键位映射到 2~9 数字编码，再枚举合法拼音
 *   - 功能键（切换模式 / 确认 / 左右翻页等）
 */
static void lv_ime_pinyin_kb_event(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * kb = lv_event_get_target(e);         /* 触发事件的键盘对象 */
    lv_obj_t * obj = lv_event_get_user_data(e);     /* 绑定时传入的输入法对象 */

    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

#if LV_IME_PINYIN_USE_K9_MODE
    /* K9 九宫格按键编号(数字键 2~9)到对应字母集合的映射表 */
    static const char * k9_py_map[8] = {"abc", "def", "ghi", "jkl", "mno", "pqrs", "tuv", "wxyz"};
#endif

    if(code == LV_EVENT_VALUE_CHANGED) {
        uint16_t btn_id  = lv_btnmatrix_get_selected_btn(kb);   /* 被按下按键的索引 */
        if(btn_id == LV_BTNMATRIX_BTN_NONE) return;             /* 无按键被按下则直接返回 */

        const char * txt = lv_btnmatrix_get_btn_text(kb, lv_btnmatrix_get_selected_btn(kb));
        if(txt == NULL) return;                                  /* 按键文本为空则直接返回 */

#if LV_IME_PINYIN_USE_K9_MODE
        if(pinyin_ime->mode == LV_IME_PINYIN_MODE_K9) {
            lv_obj_t * ta = lv_keyboard_get_textarea(pinyin_ime->kb);
            uint16_t tmp_btn_str_len = strlen(pinyin_ime->input_char);
            if((btn_id >= 16) && (tmp_btn_str_len > 0) && (btn_id < (16 + LV_IME_PINYIN_K9_CAND_TEXT_NUM))) {
                tmp_btn_str_len = strlen(pinyin_ime->input_char);
                lv_memset_00(pinyin_ime->input_char, sizeof(pinyin_ime->input_char));
                strcat(pinyin_ime->input_char, txt);
                pinyin_input_proc(obj);

                for(int index = 0; index < (pinyin_ime->ta_count + tmp_btn_str_len); index++) {
                    lv_textarea_del_char(ta);
                }

                pinyin_ime->ta_count = tmp_btn_str_len;
                pinyin_ime->k9_input_str_len = tmp_btn_str_len;
                lv_textarea_add_text(ta, pinyin_ime->input_char);

                return;
            }
        }
#endif

        /* 回车键，空格: 结束本次输入，清空输入法内部数据 */
        if(strcmp(txt, "Enter") == 0 || strcmp(txt, LV_SYMBOL_NEW_LINE) == 0) {
            pinyin_ime_clear_data(obj);
        }
        else if(strcmp(txt, " ") == 0) {
            pinyin_ime_clear_data(obj);
        }
        /* 退格键: 删除输入缓冲区中的最后一个字符 */
        else if(strcmp(txt, LV_SYMBOL_BACKSPACE) == 0) {
            if(pinyin_ime->ta_count > 0) {
                /* K26 模式删除拼音串末尾字符 */
                if(pinyin_ime->mode == LV_IME_PINYIN_MODE_K26)
                    pinyin_ime->input_char[pinyin_ime->ta_count - 1] = '\0';
                /* K9 模式删除数字编码串末尾字符 */
#if LV_IME_PINYIN_USE_K9_MODE
                else if (pinyin_ime->mode == LV_IME_PINYIN_MODE_K9)
                    pinyin_ime->k9_input_str[pinyin_ime->ta_count - 1] = '\0';
#endif

                pinyin_ime->ta_count--;
                /* 输入缓冲区已清空: 隐藏候选面板，重置候选区 */
                if(pinyin_ime->ta_count <= 0) {
                    pinyin_ime_clear_data(obj);
                }
                /* K26 模式: 还有剩余拼音，重新检索候选字 */
                else if(pinyin_ime->mode == LV_IME_PINYIN_MODE_K26) {
                    pinyin_input_proc(obj);
                }
                /* K9 模式: 重新枚举剩余数字编码对应的合法拼音并刷新候选 */
#if LV_IME_PINYIN_USE_K9_MODE
                else if(pinyin_ime->mode == LV_IME_PINYIN_MODE_K9) {
                    pinyin_ime->k9_input_str_len = strlen(pinyin_ime->input_char) - 1;
                    pinyin_k9_get_legal_py(obj, pinyin_ime->k9_input_str, k9_py_map);
                    pinyin_k9_fill_cand(obj);
                    pinyin_input_proc(obj);
                    pinyin_ime->ta_count--;
                }
#endif
            }
        }
        /* 大小写切换键 / K9 数字键 "1#" */
        else if((strcmp(txt, "ABC") == 0) || (strcmp(txt, "abc") == 0) || (strcmp(txt, "1#") == 0)) {
            pinyin_ime_clear_data(obj);
            return;
        }
        /* 键盘切换键: 在 K26 与 K9 之间切换 */
        else if(strcmp(txt, LV_SYMBOL_KEYBOARD) == 0) {
            if(pinyin_ime->mode == LV_IME_PINYIN_MODE_K26) {
                lv_ime_pinyin_set_mode(obj, LV_IME_PINYIN_MODE_K9);
            }
            else {
                lv_ime_pinyin_set_mode(obj, LV_IME_PINYIN_MODE_K26);
                lv_keyboard_set_mode(pinyin_ime->kb, LV_KEYBOARD_MODE_TEXT_LOWER);
            }
            pinyin_ime_clear_data(obj);
        }
        /* 确认键: 结束输入，清空数据 */
        else if(strcmp(txt, LV_SYMBOL_OK) == 0) {
            pinyin_ime_clear_data(obj);
        }
        /* K26 模式下的字母键: 把字母追加到拼音串末尾，然后检索候选字 */
        else if((pinyin_ime->mode == LV_IME_PINYIN_MODE_K26) && (txt[0] >= 'a' && txt[0] <= 'z')) {
            strcat(pinyin_ime->input_char, txt);
            pinyin_input_proc(obj);
            pinyin_ime->ta_count++;
        }
        /* K9 模式下的数字键(字母组)输入: 按键映射为数字编码 2~9 存入 k9_input_str */
#if LV_IME_PINYIN_USE_K9_MODE
        else if((pinyin_ime->mode == LV_IME_PINYIN_MODE_K9) && (txt[0] >= 'a' && txt[0] <= 'z')) {
            for(uint16_t i = 0; i < 8; i++) {
                if((strcmp(txt, k9_py_map[i]) == 0) || (strcmp(txt, "abc ") == 0)) {
                    /* "abc " 带空格表示作为独立的拼音分隔；否则累加当前键的字母数 */
                    if(strcmp(txt, "abc ") == 0)    pinyin_ime->k9_input_str_len += strlen(k9_py_map[i]) + 1;
                    else                            pinyin_ime->k9_input_str_len += strlen(k9_py_map[i]);
                    /* '2' 的 ASCII 码为 50，故 50 + i 得到按键编号 2~9 */
                    pinyin_ime->k9_input_str[pinyin_ime->ta_count] = 50 + i;

                    break;
                }
            }
            pinyin_k9_get_legal_py(obj, pinyin_ime->k9_input_str, k9_py_map);
            pinyin_k9_fill_cand(obj);
            pinyin_input_proc(obj);
        }
        /* K9 模式左右翻页键: dir=0 上一页, dir=1 下一页 */
        else if(strcmp(txt, LV_SYMBOL_LEFT) == 0) {
            pinyin_k9_cand_page_proc(obj, 0);
        }
        else if(strcmp(txt, LV_SYMBOL_RIGHT) == 0) {
            pinyin_k9_cand_page_proc(obj, 1);
        }
#endif
        else {
            pinyin_ime_clear_data(obj);
        }
    }
}

/**
 * 候选面板按键事件回调。
 * 候选面板是一个按钮矩阵，首尾两个按钮分别是 "<"(上一页) 和 ">"(下一页)，
 * 中间的按钮是候选汉字。用户点击候选汉字后，用该字替换 textarea 中的拼音串。
 */
static void lv_ime_pinyin_cand_panel_event(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * cand_panel = lv_event_get_target(e);   /* 候选面板对象 */
    lv_obj_t * obj = (lv_obj_t *)lv_event_get_user_data(e);

    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    if(code == LV_EVENT_VALUE_CHANGED) {
        uint32_t id = lv_btnmatrix_get_selected_btn(cand_panel);
        /* 点击 "<": 翻到上一页候选 */
        if(id == 0) {
            pinyin_page_proc(obj, 0);
            return;
        }
        /* 点击 ">": 翻到下一页候选 */
        if(id == (LV_IME_PINYIN_CAND_TEXT_NUM + 1)) {
            pinyin_page_proc(obj, 1);
            return;
        }

        /* 点击了某个候选汉字: 把拼音串从 textarea 中删除，替换为选中的汉字 */
        const char * txt = lv_btnmatrix_get_btn_text(cand_panel, id);
        lv_obj_t * ta = lv_keyboard_get_textarea(pinyin_ime->kb);
        uint16_t index = 0;
        for(index = 0; index < pinyin_ime->ta_count; index++)
            lv_textarea_del_char(ta);   /* 逐个删除当前已输入的拼音字母 */

        lv_textarea_add_text(ta, txt);   /* 把选中的汉字写入 textarea */

        pinyin_ime_clear_data(obj);      /* 一次选择完成，清空输入法内部数据 */
    }
}

/**
 * 拼音输入处理（K26 全键盘模式的核心检索逻辑）。
 * 1. 在字典中查找以当前 input_char 为前缀的条目；
 * 2. 若找到，则把该条目的候选汉字字符串按每 3 字节(一个 UTF-8 汉字)拆分，
 *    填入候选面板的按钮数组，并显示候选面板。
 */
static void pinyin_input_proc(lv_obj_t * obj)
{
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    /* 检索字典：cand_str 指向匹配条目的汉字串，cand_num 为该串中的汉字个数 */
    pinyin_ime->cand_str = pinyin_search_matching(obj, pinyin_ime->input_char, &pinyin_ime->cand_num);
    if(pinyin_ime->cand_str == NULL) {
        return;   /* 未找到匹配，保持候选面板不变 */
    }

    pinyin_ime->py_page = 0;   /* 每次重新检索后回到第一页 */

    /* 清空候选按钮的显示缓冲，并为每个候选位置预填一个空格占位 */
    for(uint8_t i = 0; i < LV_IME_PINYIN_CAND_TEXT_NUM; i++) {
        memset(lv_pinyin_cand_str[i], 0x00, sizeof(lv_pinyin_cand_str[i]));
        lv_pinyin_cand_str[i][0] = ' ';
    }

    /* 把匹配到的汉字按顺序填入候选按钮(每个汉字 3 字节，最多填满一屏) */
    for(uint8_t i = 0; (i < pinyin_ime->cand_num && i < LV_IME_PINYIN_CAND_TEXT_NUM); i++) {
        for(uint8_t j = 0; j < 3; j++) {
            lv_pinyin_cand_str[i][j] = pinyin_ime->cand_str[i * 3 + j];
        }
    }

    lv_obj_clear_flag(pinyin_ime->cand_panel, LV_OBJ_FLAG_HIDDEN);   /* 显示候选面板 */
}

/**
 * 候选字翻页处理。
 * @param dir 0 = 上一页，1 = 下一页
 * 计算总页数(page_num)和最后一页剩余的候选数(sur)，
 * 更新 py_page 后按当前页偏移重新填充候选按钮。
 */
static void pinyin_page_proc(lv_obj_t * obj, uint16_t dir)
{
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;
    uint16_t page_num = pinyin_ime->cand_num / LV_IME_PINYIN_CAND_TEXT_NUM;   /* 完整页数 */
    uint16_t sur = pinyin_ime->cand_num % LV_IME_PINYIN_CAND_TEXT_NUM;        /* 最后一页剩余候选数 */

    if(dir == 0) {
        /* 上一页：页号减一(下限为 0) */
        if(pinyin_ime->py_page) {
            pinyin_ime->py_page--;
        }
    }
    else {
        /* 下一页：先修正总页数(无余数时减一)，再判断是否还能翻页 */
        if(sur == 0) {
            page_num -= 1;
        }
        if(pinyin_ime->py_page < page_num) {
            pinyin_ime->py_page++;
        }
        else return;   /* 已是最后一页，无法继续翻页 */
    }

    /* 清空候选显示缓冲 */
    for(uint8_t i = 0; i < LV_IME_PINYIN_CAND_TEXT_NUM; i++) {
        memset(lv_pinyin_cand_str[i], 0x00, sizeof(lv_pinyin_cand_str[i]));
        lv_pinyin_cand_str[i][0] = ' ';
    }

    /* 按当前页偏移重新填充候选汉字 */
    uint16_t offset = pinyin_ime->py_page * (3 * LV_IME_PINYIN_CAND_TEXT_NUM);
    for(uint8_t i = 0; (i < pinyin_ime->cand_num && i < LV_IME_PINYIN_CAND_TEXT_NUM); i++) {
        /* 最后一页且有余数时，只填充到 sur 为止，避免越界读 */
        if((sur > 0) && (pinyin_ime->py_page == page_num)) {
            if(i > sur)
                break;
        }
        for(uint8_t j = 0; j < 3; j++) {
            lv_pinyin_cand_str[i][j] = pinyin_ime->cand_str[offset + (i * 3) + j];
        }
    }
}

static void lv_ime_pinyin_style_change_event(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    if(code == LV_EVENT_STYLE_CHANGED) {
        const lv_font_t * font = lv_obj_get_style_text_font(obj, LV_PART_MAIN);
        lv_obj_set_style_text_font(pinyin_ime->cand_panel, font, 0);
    }
}

/**
 * 初始化字典并建立首字母索引。
 * 遍历字典(以 NULL 条目结尾)，按首字母 a~z 分组统计：
 *   - py_num[x]  记录首字母为 ('a'+x) 的条目数量
 *   - py_pos[x]  记录首字母为 ('a'+x) 的第一个条目在字典中的偏移
 * 后续 pinyin_search_matching 通过这些索引快速定位到对应首字母的区段，
 * 从而避免每次都从头线性扫描整个字典。
 */
static void init_pinyin_dict(lv_obj_t * obj, lv_pinyin_dict_t * dict)
{
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    char headletter = 'a';          /* 当前正在统计的首字母 */
    uint16_t offset_sum = 0;        /* 已统计条目的累计总数，用于计算区段起始偏移 */
    uint16_t offset_count = 0;      /* 当前首字母区段内的条目计数 */
    uint16_t letter_calc = 0;       /* 首字母映射到数组下标的临时变量 */

    pinyin_ime->dict = dict;

    for(uint16_t i = 0; ; i++) {
        /* 遇到结尾哨兵条目(NULL)，记录最后一个字母区段的条目数并结束 */
        if((NULL == (dict[i].py)) || (NULL == (dict[i].py_mb))) {
            headletter = dict[i - 1].py[0];
            letter_calc = headletter - 'a';
            pinyin_ime->py_num[letter_calc] = offset_count;
            break;
        }

        /* 当前条目首字母与上一分组相同，计数加一 */
        if(headletter == (dict[i].py[0])) {
            offset_count++;
        }
        else {
            /* 遇到新的首字母：结算上一分组，并记录新分组的起始偏移 */
            headletter = dict[i].py[0];
            letter_calc = headletter - 'a';
            pinyin_ime->py_num[letter_calc - 1] = offset_count;   /* 上一字母区段的条目数 */
            offset_sum += offset_count;
            pinyin_ime->py_pos[letter_calc] = offset_sum;          /* 当前字母区段的起始偏移 */

            offset_count = 1;   /* 当前条目计入新分组 */
        }
    }
}

/**
 * 在字典中查找以 py_str 为前缀的拼音条目。
 * 利用 init_pinyin_dict 预先建立的首字母索引(py_num/py_pos)快速定位
 * 到 py_str 首字母对应的字典区段，然后线性扫描该区段做前缀匹配。
 *
 * @param obj     输入法对象
 * @param py_str  待匹配的拼音串(如 "zhong")
 * @param cand_num 输出参数，返回匹配条目所包含的汉字个数
 * @return        匹配条目的汉字字符串指针，未找到返回 NULL
 */
static char * pinyin_search_matching(lv_obj_t * obj, char * py_str, uint16_t * cand_num)
{
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    lv_pinyin_dict_t * cpHZ;
    uint8_t index, len = 0, offset;
    volatile uint8_t count = 0;

    /* 空串以及 i/u/v 开头的串无对应拼音，直接返回 */
    if(*py_str == '\0')    return NULL;
    if(*py_str == 'i')     return NULL;
    if(*py_str == 'u')     return NULL;
    if(*py_str == 'v')     return NULL;

    /* 用首字母定位到字典区段：offset = 首字母 - 'a' */
    offset = py_str[0] - 'a';
    len = strlen(py_str);

    cpHZ  = &pinyin_ime->dict[pinyin_ime->py_pos[offset]];   /* 该首字母区段的起始条目 */
    count = pinyin_ime->py_num[offset];                      /* 该首字母区段的条目数量 */

    while(count--) {
        /* 逐字符比较 py_str 与当前条目的拼音，做前缀匹配 */
        for(index = 0; index < len; index++) {
            if(*(py_str + index) != *((cpHZ->py) + index)) {
                break;   /* 出现不匹配字符，提前退出 */
            }
        }

        /* 完全匹配(单字母输入或逐字符全部匹配成功) */
        if(len == 1 || index == len) {
            /* UTF-8 编码下每个汉字占 3 字节，因此汉字个数 = 字符串字节长度 / 3 */
            * cand_num = strlen((const char *)(cpHZ->py_mb)) / 3;
            return (char *)(cpHZ->py_mb);
        }
        cpHZ++;   /* 移动到下一个字典条目 */
    }
    return NULL;
}

/**
 * 清空输入法内部状态：重置计数、清空输入缓冲与候选缓冲，并隐藏候选面板。
 * 通常在完成一次选字、回车、确认或切换键盘模式时调用。
 */
static void pinyin_ime_clear_data(lv_obj_t * obj)
{
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

#if LV_IME_PINYIN_USE_K9_MODE
    /* K9 模式额外需要清理九宫格编码串与合法拼音链表相关状态 */
    if(pinyin_ime->mode == LV_IME_PINYIN_MODE_K9) {
        pinyin_ime->k9_input_str_len = 0;
        pinyin_ime->k9_py_ll_pos = 0;
        pinyin_ime->k9_legal_py_count = 0;
        lv_memset_00(pinyin_ime->k9_input_str,  LV_IME_PINYIN_K9_MAX_INPUT);
        lv_memset_00(lv_pinyin_k9_cand_str, sizeof(lv_pinyin_k9_cand_str));
        strcpy(lv_pinyin_k9_cand_str[LV_IME_PINYIN_K9_CAND_TEXT_NUM], LV_SYMBOL_RIGHT"\0");
        strcpy(lv_pinyin_k9_cand_str[LV_IME_PINYIN_K9_CAND_TEXT_NUM + 1], "\0");
    }
#endif

    pinyin_ime->ta_count = 0;
    lv_memset_00(lv_pinyin_cand_str, (sizeof(lv_pinyin_cand_str)));
    lv_memset_00(pinyin_ime->input_char, sizeof(pinyin_ime->input_char));

    lv_obj_add_flag(pinyin_ime->cand_panel, LV_OBJ_FLAG_HIDDEN);
}

#if LV_IME_PINYIN_USE_K9_MODE
static void pinyin_k9_init_data(lv_obj_t * obj)
{
    //lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    uint16_t py_str_i = 0;
    uint16_t btnm_i = 0;
    for(btnm_i = 19; btnm_i < (LV_IME_PINYIN_K9_CAND_TEXT_NUM + 21); btnm_i++) {
        if(py_str_i == LV_IME_PINYIN_K9_CAND_TEXT_NUM) {
            strcpy(lv_pinyin_k9_cand_str[py_str_i], LV_SYMBOL_RIGHT"\0");
        }
        else if(py_str_i == LV_IME_PINYIN_K9_CAND_TEXT_NUM + 1) {
            strcpy(lv_pinyin_k9_cand_str[py_str_i], "\0");
        }
        else {
            strcpy(lv_pinyin_k9_cand_str[py_str_i], " \0");
        }

        lv_btnm_def_pinyin_k9_map[btnm_i] = lv_pinyin_k9_cand_str[py_str_i];
        py_str_i++;
    }

    default_kb_ctrl_k9_map[0]  = LV_KEYBOARD_CTRL_BTN_FLAGS | 1;
    default_kb_ctrl_k9_map[4]  = LV_KEYBOARD_CTRL_BTN_FLAGS | 1;
    default_kb_ctrl_k9_map[5]  = LV_KEYBOARD_CTRL_BTN_FLAGS | 1;
    default_kb_ctrl_k9_map[9]  = LV_KEYBOARD_CTRL_BTN_FLAGS | 1;
    default_kb_ctrl_k9_map[10] = LV_KEYBOARD_CTRL_BTN_FLAGS | 1;
    default_kb_ctrl_k9_map[14] = LV_KEYBOARD_CTRL_BTN_FLAGS | 1;
    default_kb_ctrl_k9_map[15] = LV_KEYBOARD_CTRL_BTN_FLAGS | 1;
    default_kb_ctrl_k9_map[LV_IME_PINYIN_K9_CAND_TEXT_NUM + 16] = LV_KEYBOARD_CTRL_BTN_FLAGS | 1;
}

/**
 * K9 九宫格模式：根据用户按下的数字键序列(k9_input)，枚举所有可能的拼音组合。
 * 每个数字键(2~9)对应 3~4 个字母(py9_map)，使用回溯法生成所有字母组合，
 * 并调用 pinyin_k9_is_valid_py 过滤出字典中真实存在的合法拼音，
 * 合法的拼音串通过链表 k9_legal_py_ll 保存，供候选填充使用。
 */
static void pinyin_k9_get_legal_py(lv_obj_t * obj, char * k9_input, const char * py9_map[])
{
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    uint16_t len = strlen(k9_input);

    if((len == 0) || (len >= LV_IME_PINYIN_K9_MAX_INPUT)) {
        return;   /* 空输入或超过最大长度，直接返回 */
    }

    char py_comp[LV_IME_PINYIN_K9_MAX_INPUT] = {0};   /* 当前正在组合的拼音缓冲区 */
    int mark[LV_IME_PINYIN_K9_MAX_INPUT] = {0};       /* 记录每一位数字键已尝试到第几个字母 */
    int index = 0;                                     /* 当前正在填写的位数 */
    int flag = 0;
    int count = 0;                                     /* 已找到的合法拼音个数 */

    uint32_t ll_len = 0;
    ime_pinyin_k9_py_str_t * ll_index = NULL;

    ll_len = _lv_ll_get_len(&pinyin_ime->k9_legal_py_ll);
    ll_index = _lv_ll_get_head(&pinyin_ime->k9_legal_py_ll);

    /* 回溯法枚举：index 表示当前处理到第几位，-1 表示回溯结束 */
    while(index != -1) {
        if(index == len) {
            /* 已填满所有位，得到一个完整拼音组合，校验其是否为合法拼音 */
            if(pinyin_k9_is_valid_py(obj, py_comp)) {
                if((count >= ll_len) || (ll_len == 0)) {
                    /* 链表容量不足则在尾部新建节点 */
                    ll_index = _lv_ll_ins_tail(&pinyin_ime->k9_legal_py_ll);
                    strcpy(ll_index->py_str, py_comp);
                }
                else if((count < ll_len)) {
                    /* 复用已有节点，覆盖旧的拼音串 */
                    strcpy(ll_index->py_str, py_comp);
                    ll_index = _lv_ll_get_next(&pinyin_ime->k9_legal_py_ll, ll_index);
                }
                count++;
            }
            index--;   /* 回溯到上一位 */
        }
        else {
            flag = mark[index];
            if(flag < strlen(py9_map[k9_input[index] - '2'])) {
                /* 该位还有未尝试的字母，取当前字母继续向低位推进 */
                py_comp[index] = py9_map[k9_input[index] - '2'][flag];
                mark[index] = mark[index] + 1;
                index++;
            }
            else {
                /* 该位字母已穷尽，重置标记并回溯到上一位 */
                mark[index] = 0;
                index--;
            }
        }
    }

    /* 找到合法拼音后，更新已输入字符计数与合法拼音总数 */
    if(count > 0) {
        pinyin_ime->ta_count++;
        pinyin_ime->k9_legal_py_count = count;
    }
}

/**
 * 校验一个拼音串是否存在于字典中(前缀匹配)。
 * 实现与 pinyin_search_matching 类似，但只返回是否存在(true/false)。
 */
static bool pinyin_k9_is_valid_py(lv_obj_t * obj, char * py_str)
{
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    lv_pinyin_dict_t * cpHZ = NULL;
    uint8_t index = 0, len = 0, offset = 0;
    volatile uint8_t count = 0;

    if(*py_str == '\0')    return false;
    if(*py_str == 'i')     return false;
    if(*py_str == 'u')     return false;
    if(*py_str == 'v')     return false;

    offset = py_str[0] - 'a';
    len = strlen(py_str);

    cpHZ  = &pinyin_ime->dict[pinyin_ime->py_pos[offset]];
    count = pinyin_ime->py_num[offset];

    while(count--) {
        for(index = 0; index < len; index++) {
            if(*(py_str + index) != *((cpHZ->py) + index)) {
                break;
            }
        }

        if(len == 1 || index == len) {
            return true;   /* 前缀匹配成功 */
        }
        cpHZ++;
    }
    return false;
}

/**
 * K9 模式：把合法拼音链表的头几个拼音填充到候选按钮，
 * 并把第一个合法拼音写入 input_char、同步到 textarea 作为预览。
 */
static void pinyin_k9_fill_cand(lv_obj_t * obj)
{
    static uint16_t len = 0;   /* 上次填充时的合法拼音总数，用于判断是否需要重填 */
    uint16_t index = 0, tmp_len = 0;
    ime_pinyin_k9_py_str_t * ll_index = NULL;

    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    tmp_len = pinyin_ime->k9_legal_py_count;

    /* 合法拼音总数发生变化时，清空候选缓冲并重置翻页按钮 */
    if(tmp_len != len) {
        lv_memset_00(lv_pinyin_k9_cand_str, sizeof(lv_pinyin_k9_cand_str));
        strcpy(lv_pinyin_k9_cand_str[LV_IME_PINYIN_K9_CAND_TEXT_NUM], LV_SYMBOL_RIGHT"\0");
        strcpy(lv_pinyin_k9_cand_str[LV_IME_PINYIN_K9_CAND_TEXT_NUM + 1], "\0");
        len = tmp_len;
    }

    /* 从头遍历链表，把拼音串填入候选按钮(最多一屏) */
    ll_index = _lv_ll_get_head(&pinyin_ime->k9_legal_py_ll);
    strcpy(pinyin_ime->input_char, ll_index->py_str);   /* 默认取第一个拼音作为预览 */
    while(ll_index) {
        if((index >= LV_IME_PINYIN_K9_CAND_TEXT_NUM) || \
           (index >= pinyin_ime->k9_legal_py_count))
            break;

        strcpy(lv_pinyin_k9_cand_str[index], ll_index->py_str);
        ll_index = _lv_ll_get_next(&pinyin_ime->k9_legal_py_ll, ll_index);
        index++;
    }
    pinyin_ime->k9_py_ll_pos = index;   /* 记录当前候选填充到的链表位置 */

    /* 同步预览：先删掉 textarea 中旧的编码串，再写入新的合法拼音 */
    lv_obj_t * ta = lv_keyboard_get_textarea(pinyin_ime->kb);
    for(index = 0; index < pinyin_ime->k9_input_str_len; index++) {
        lv_textarea_del_char(ta);
    }
    pinyin_ime->k9_input_str_len = strlen(pinyin_ime->input_char);
    lv_textarea_add_text(ta, pinyin_ime->input_char);
}

/**
 * K9 模式候选翻页。
 * @param dir 0 = 上一页，1 = 下一页
 * 通过 k9_py_ll_pos 记录当前页在合法拼音链表中的位置，
 * 翻页时从链表对应位置重新取出一屏拼音填入候选按钮。
 */
static void pinyin_k9_cand_page_proc(lv_obj_t * obj, uint16_t dir)
{
    lv_ime_pinyin_t * pinyin_ime = (lv_ime_pinyin_t *)obj;

    lv_obj_t * ta = lv_keyboard_get_textarea(pinyin_ime->kb);
    uint16_t ll_len =  _lv_ll_get_len(&pinyin_ime->k9_legal_py_ll);

    /* 只有合法拼音超过一屏时才需要翻页 */
    if((ll_len > LV_IME_PINYIN_K9_CAND_TEXT_NUM) && (pinyin_ime->k9_legal_py_count > LV_IME_PINYIN_K9_CAND_TEXT_NUM)) {
        ime_pinyin_k9_py_str_t * ll_index = NULL;
        int count = 0;

        /* 先把链表指针定位到当前页的起始位置(k9_py_ll_pos) */
        ll_index = _lv_ll_get_head(&pinyin_ime->k9_legal_py_ll);
        while(ll_index) {
            if(count >= pinyin_ime->k9_py_ll_pos)   break;

            ll_index = _lv_ll_get_next(&pinyin_ime->k9_legal_py_ll, ll_index);
            count++;
        }

        /* 已在链表末尾却还要翻下一页，直接返回 */
        if((NULL == ll_index) && (dir == 1))   return;

        /* 清空候选缓冲并重置翻页按钮 */
        lv_memset_00(lv_pinyin_k9_cand_str, sizeof(lv_pinyin_k9_cand_str));
        strcpy(lv_pinyin_k9_cand_str[LV_IME_PINYIN_K9_CAND_TEXT_NUM], LV_SYMBOL_RIGHT"\0");
        strcpy(lv_pinyin_k9_cand_str[LV_IME_PINYIN_K9_CAND_TEXT_NUM + 1], "\0");

        // next page
        if(dir == 1) {
            /* 下一页：从当前位置向后取一屏拼音 */
            count = 0;
            while(ll_index) {
                if(count >= (LV_IME_PINYIN_K9_CAND_TEXT_NUM - 1))
                    break;

                strcpy(lv_pinyin_k9_cand_str[count], ll_index->py_str);
                ll_index = _lv_ll_get_next(&pinyin_ime->k9_legal_py_ll, ll_index);
                count++;
            }
            pinyin_ime->k9_py_ll_pos += count - 1;
        }
        // previous page
        else {
            /* 上一页：从当前位置向前回退一屏拼音 */
            count = LV_IME_PINYIN_K9_CAND_TEXT_NUM - 1;
            ll_index = _lv_ll_get_prev(&pinyin_ime->k9_legal_py_ll, ll_index);
            while(ll_index) {
                if(count < 0)  break;

                strcpy(lv_pinyin_k9_cand_str[count], ll_index->py_str);
                ll_index = _lv_ll_get_prev(&pinyin_ime->k9_legal_py_ll, ll_index);
                count--;
            }

            if(pinyin_ime->k9_py_ll_pos > LV_IME_PINYIN_K9_CAND_TEXT_NUM)
                pinyin_ime->k9_py_ll_pos -= 1;
        }

        lv_textarea_set_cursor_pos(ta, LV_TEXTAREA_CURSOR_LAST);
    }
}

#endif  /*LV_IME_PINYIN_USE_K9_MODE*/

#endif  /*LV_USE_IME_PINYIN*/