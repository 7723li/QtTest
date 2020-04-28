import ExcelExport

def test():
    sheetname = "sheet1"
    ExcelExport.init(sheetname)

    ExcelExport.header()

    index = 0
    PatientMSG = ["examdate.c_str()",  "examdate.c_str()", 123, "sdkhf", "sdsdlfk", "sdkhfksdhf", 999]
    commonTab = [34.99, 8.123, 41.300, 8.930, 3.000, 0.000]
    EUTab = [100.0, 100.0, 41.300, 8.930]
    MSTab = [100.0, 100.0, 41.124, 8.896, 3.000, 0.000, 100.0, 100.0, 41.003, 8.880, 3.000, 0.000]

    for i in range(10):
        ExcelExport.data(i, PatientMSG, commonTab, EUTab, MSTab)
        
    file = "E:\\TopV\\V100\\V2.85+\\TopV2\\export_Excel\\20191016-163937.xls"
    return ExcelExport.final(file)

print(test())
