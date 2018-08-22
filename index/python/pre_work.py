# coding:utf-8
import os
import re
from bs4 import BeautifulSoup
import sys
reload(sys)
sys.setdefaultencoding('utf-8')

input_path = '../data/input/'
output_path = '../data/tmp/raw_input'
url_prefix = 'https://www.boost.org/doc/libs/1_53_0/doc/'

##过滤HTML中的标签
#将HTML中标签等信息去掉
#@param htmlstr HTML字符串.
def filter_tags(htmlstr):
    #先过滤CDATA
    re_cdata=re.compile('//<!\[CDATA\[[^>]*//\]\]>',re.I) #匹配CDATA
    re_script=re.compile('<\s*script[^>]*>[^<]*<\s*/\s*script\s*>',re.I)#Script
    re_style=re.compile('<\s*style[^>]*>[^<]*<\s*/\s*style\s*>',re.I)#style
    re_br=re.compile('<br\s*?/?>')#处理换行
    re_h=re.compile('</?\w+[^>]*>')#HTML标签
    re_comment=re.compile('<!--[^>]*-->')#HTML注释
    s=re_cdata.sub('',htmlstr)#去掉CDATA
    s=re_script.sub('',s) #去掉SCRIPT
    s=re_style.sub('',s)#去掉style

    # 此处由于需要让一个文档只占一行, 把 <br> 替换成空格
    s=re_br.sub(' ',s)

    s=re_h.sub('',s) #去掉HTML 标签
    s=re_comment.sub('',s)#去掉HTML注释
    #去掉多余的空行
    blank_line=re.compile('\n+')

    # 此处把多个空行合并成一个空格
    s=blank_line.sub(' ',s)

    s=replaceCharEntity(s)#替换实体
    return s

##替换常用HTML字符实体.
#使用正常的字符替换HTML中特殊的字符实体.
#你可以添加新的实体字符到CHAR_ENTITIES中,处理更多HTML字符实体.
#@param htmlstr HTML字符串.
def replaceCharEntity(htmlstr):
    CHAR_ENTITIES={'nbsp':' ','160':' ',
                   'lt':'<','60':'<',
                   'gt':'>','62':'>',
                   'amp':'&','38':'&',
                   'quot':'"','34':'"',}

    re_charEntity=re.compile(r'&#?(?P<name>\w+);')
    sz=re_charEntity.search(htmlstr)
    while sz:
        key=sz.group('name')#去除&;后entity,如&gt;为gt
        try:
            htmlstr=re_charEntity.sub(CHAR_ENTITIES[key],htmlstr,1)
            sz=re_charEntity.search(htmlstr)
        except KeyError:
            #以空串代替
            htmlstr=re_charEntity.sub('',htmlstr,1)
            sz=re_charEntity.search(htmlstr)
    return htmlstr


def enum_file(input_path):
    '枚举目录中的所有子目录和文件'
    file_list = []
    for basedir, dirnames, filenames in os.walk(input_path):
        for f in filenames:
            # 只关注扩展名为 html 的文件
            if os.path.splitext(f)[-1] != '.html':
                continue
            file_list.append(basedir + '/' + f)
    return file_list


def parse_url(file_path):
    # input_path = ../data/input/
    # file_path = ../data/input/html/intrusive/list.html
    return url_prefix + file_path[len(input_path):]


def parse_title(html):
    soup = BeautifulSoup(html, 'html.parser')
    return soup.find('title').string


def parse_content(html):
    return filter_tags(html)


def parse_file(file_path):
    '得到的结果是一个三元组: jump_url, title, content'
    html = open(file_path).read()
    return parse_url(file_path), parse_title(html), parse_content(html)


def write_result(result, output_file):
    '把三元组当做一行写入到输出文件中'
    if result[0] and result[1] and result[2]:
        output_file.write(result[0] + '\3' + result[1] + '\3' + result[2] + '\n')


def run():
    '预处理操作的入口'
    # 1. 遍历 input_path 所有的文件和路径
    file_list = enum_file(input_path)
    with open(output_path, 'w') as output_file:
        # 2. 针对每一个文件, 解析其中的内容
        for f in file_list:
            result = parse_file(f)
            # 3. 把内容写入到最终的输出结果中
            write_result(result, output_file)

if __name__ == '__main__':
    run()
