#ifndef PINYIN_IME_H
#define PINYIN_IME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* 不透明句柄，一次初始化对应一个独立的输入法实例 */
typedef struct pinyin_ime_t pinyin_ime_t;

/* 返回码 */
#define PINYIN_IME_OK               0
#define PINYIN_IME_ERR_INVALID_ARG -1
#define PINYIN_IME_ERR_BAD_PINYIN  -2
#define PINYIN_IME_ERR_INDEX       -3
#define PINYIN_IME_ERR_IO          -4

/*
 * 初始化：指定拼音表文件（如 pinyin.txt）与词典文件（如 dictionary.data）。
 * 成功返回句柄，失败返回 NULL。
 */
pinyin_ime_t* pinyin_ime_init(const char* pinyin_path, const char* dictionary_path);

/*
 * 销毁实例并释放内存。销毁后所有通过访问器获取的字符串指针失效。
 */
void pinyin_ime_destroy(pinyin_ime_t* ime);

/*
 * 输入拼音字符串（全小写，可用 ' 分隔音节），清空当前缓存并生成新的
 * 分词结果与候选词列表。成功返回 PINYIN_IME_OK。
 * 结果通过 pinyin_ime_get_segments / pinyin_ime_get_candidate* 获取。
 */
int pinyin_ime_input(pinyin_ime_t* ime, const char* pinyin);

/*
 * 选择候选词，序号从 0 开始。选中后推进剩余分词，并生成下一批候选词。
 * 序号越界返回 PINYIN_IME_ERR_INDEX。
 */
int pinyin_ime_select(pinyin_ime_t* ime, uint32_t index);

/*
 * 保存词库。dictionary_path 传 NULL 则写回初始化时的词典路径。
 */
int pinyin_ime_save(pinyin_ime_t* ime, const char* dictionary_path);

/*
 * 获取当前剩余的拼音分词结果（UTF-8，如 "shu'li'kou"）。
 * 简拼/半简拼半全拼模式下，未完成时为原始输入串，完成后为空串。
 */
const char* pinyin_ime_get_segments(pinyin_ime_t* ime);

/* 获取目前已选中的汉字结果（UTF-8） */
const char* pinyin_ime_get_result(pinyin_ime_t* ime);

/* 获取当前候选词数量 */
int pinyin_ime_get_candidate_count(pinyin_ime_t* ime);

/* 获取第 index 个候选词（UTF-8），越界返回 NULL */
const char* pinyin_ime_get_candidate(pinyin_ime_t* ime, uint32_t index);

/* 获取当前 k9 对应的精确拼音数量 */
int pinyin_ime_get_k9_exact_count(pinyin_ime_t* ime);

/* 获取第 index 个 k9 精确拼音，越界返回 NULL */
const char* pinyin_ime_get_k9_exact(pinyin_ime_t* ime, uint32_t index);

/* 选择第 index 个 k9 精确拼音 */
int pinyin_ime_select_k9_exact(pinyin_ime_t* ime, uint32_t index);

/* 是否已结束当前输入（无剩余分词） */
int pinyin_ime_is_finished(pinyin_ime_t* ime);

#ifdef __cplusplus
}
#endif

#endif /* PINYIN_IME_H */
