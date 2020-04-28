import os
import xlwt

'''
暂未找到C++调用Python3类的方法
暂用全局函数代替
'''

g_RowIndex = 0          #行号
g_ColIndex = 0          #列号
g_book = None           #创建新的表格
g_sheet = None          #创建新的工作表

def init(sheetname):
    assert(type(sheetname) is str)

    global g_RowIndex
    global g_ColIndex
    global g_book
    global g_sheet

    g_RowIndex = 0
    g_ColIndex = 0
    g_book = xlwt.Workbook(encoding = "utf-8")
    g_sheet = g_book.add_sheet(sheetname)

def DrawHeader():
    global g_RowIndex
    global g_ColIndex
    global g_sheet
    
    a = ["序号", "检查日期","检查时间","床号", "姓名", "住院号", "性别", "年龄", "通用参数", "EU参数", "MS参数"]
    b = ["VDDe Backer=Nx/Lg", "TVD(mm/mm2)", "MFI4", "HI4", "PPV(%)", "PVD(mm/mm2)", "PPV9(%)", "PPV9(mm/mm2)", "MFI9", "HI9", "PPV16(%)", "PPV16(mm/mm2)", "MFI16", "HI16"]
    c = [8, 9, 10, 11, 14, 15, 16, 17, 18, 19, 20, 21, 24, 25, 26, 27]

    max_len_a = max(len(x) for x in a)
    max_len_b = max(len(x) for x in b)
    max_len_c = max([len("所有血管"), len("小血管")])
    max_len = max([max_len_a, max_len_b, max_len_c])
    for i in range(1 + 7 + 6 + 4+ 12):
        g_sheet.col(i).width = 256 * max_len

    font1 = xlwt.Font()
    font1.bold = True
    font1.height = 24 * 20
    font1.name = "宋体"
    style1 = xlwt.XFStyle()
    style1.alignment.vert = 1
    style1.alignment.horz = 2
    style1.font = font1
    g_sheet.write_merge(g_RowIndex, g_RowIndex, g_ColIndex, 29, "系统所获取", style1)    
    g_RowIndex += 1

    font2 = xlwt.Font()
    font2.bold = True
    font2.height = 18 * 20
    font2.name = "宋体"
    style2 = xlwt.XFStyle()
    style2.alignment.vert = 1
    style2.alignment.horz = 2
    style2.font = font2
    g_sheet.write_merge(g_RowIndex, g_RowIndex, g_ColIndex, 29, "微循环参数导出", style2)
    g_RowIndex += 1

    font3 = xlwt.Font()
    font3.name = "宋体"
    font3.height = 16 * 20
    style3 = xlwt.XFStyle()
    style3.alignment.vert = 1
    style3.alignment.horz = 2
    style3.font = font3
    for i in range(len(a)):
        if i == 9:
            g_ColIndex += 5
        elif i == 10:
            g_ColIndex += 3

        if i <= 7:
            g_sheet.write_merge(g_RowIndex, g_RowIndex + 2, g_ColIndex, g_ColIndex, a[i], style3)
        elif i == 8:
            g_sheet.write_merge(g_RowIndex, g_RowIndex, g_ColIndex , g_ColIndex + 5, a[i], style3)
        elif i == 9:
            g_sheet.write_merge(g_RowIndex, g_RowIndex, g_ColIndex, g_ColIndex + 3, a[i], style3)
        elif i == 10:
            g_sheet.write_merge(g_RowIndex, g_RowIndex, g_ColIndex, g_ColIndex + 11, a[i], style3)
            
        g_ColIndex += 1    
    g_RowIndex += 1

    font4 = xlwt.Font()
    font4.name = "宋体"
    font4.height = 16 * 20
    style4 = xlwt.XFStyle()
    style4.font = font4
    g_ColIndex = 8
    for i in b:
        if -1 != i.find("MFI") or -1 != i.find("HI"):
            g_sheet.write_merge(g_RowIndex, g_RowIndex + 1, g_ColIndex, g_ColIndex, i, style4)
            g_ColIndex += 1
        else:
            g_sheet.write_merge(g_RowIndex, g_RowIndex, g_ColIndex, g_ColIndex + 1, i, style4)
            g_ColIndex += 2
    g_RowIndex += 1
    
    tempidx = 0
    for i in c:
        if tempidx % 2 == 0:
            g_sheet.write(g_RowIndex, i, "所有血管", style4)
        else :
            g_sheet.write(g_RowIndex, i, "小血管", style4)
        tempidx += 1
    g_RowIndex += 1    

def ExportData(index, PatientMSG, commonTab, EUTab, MSTab):
    assert(type(index) is int)
    assert(type(PatientMSG) is list and len(PatientMSG) == 7)
    assert(type(commonTab) is list and len(commonTab) == 6)
    assert(type(EUTab) is list and len(EUTab) == 4)
    assert(type(MSTab) is list and len(MSTab) == 12)

    global g_RowIndex
    global g_ColIndex
    global g_sheet

    font = xlwt.Font()
    font.height = 12 * 20
    font.name = "宋体"
    style = xlwt.XFStyle()
    style.font = font

    g_ColIndex = 0
    g_sheet.write(g_RowIndex, g_ColIndex, label = str(index))
    g_ColIndex += 1
    for i in PatientMSG:
        g_sheet.write(g_RowIndex, g_ColIndex, str(i), style)
        g_ColIndex += 1

    for i in commonTab:
        g_sheet.write(g_RowIndex, g_ColIndex, str(i), style)
        g_ColIndex += 1

    for i in EUTab:
        g_sheet.write(g_RowIndex, g_ColIndex, str(i), style)
        g_ColIndex += 1

    for i in MSTab:
        g_sheet.write(g_RowIndex, g_ColIndex, str(i), style)
        g_ColIndex += 1

    g_RowIndex += 1

def SaveFile(file):
    try:
        assert(type(file) is str)
        global g_book
        g_book.save(file)
        
        return 0

    except PermissionError:
        return -1
