object FormSetSize: TFormSetSize
  Left = 0
  Top = 0
  BorderStyle = bsDialog
  Caption = 'Select size for new nametable'
  ClientHeight = 124
  ClientWidth = 264
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  Position = poMainFormCenter
  Scaled = False
  OnCreate = FormCreate
  OnShow = FormShow
  PixelsPerInch = 96
  TextHeight = 13
  object GroupBox1: TGroupBox
    Left = 0
    Top = 0
    Width = 264
    Height = 124
    Align = alClient
    TabOrder = 0
    OnClick = RadioButtonNormalClick
    object ButtonCancel: TButton
      Left = 177
      Top = 88
      Width = 75
      Height = 25
      Caption = 'Cancel'
      TabOrder = 0
      OnClick = ButtonCancelClick
    end
    object ButtonOK: TButton
      Left = 96
      Top = 88
      Width = 75
      Height = 25
      Caption = 'OK'
      TabOrder = 1
      OnClick = ButtonOKClick
    end
    object EditWidth: TEdit
      Left = 144
      Top = 43
      Width = 33
      Height = 21
      TabOrder = 2
      Text = '32'
      OnClick = EditWidthClick
      OnExit = EditWidthExit
      OnKeyPress = EditWidthKeyPress
    end
    object EditHeight: TEdit
      Left = 202
      Top = 43
      Width = 33
      Height = 21
      TabOrder = 3
      Text = '30'
      OnClick = EditWidthClick
      OnExit = EditHeightExit
      OnKeyPress = EditWidthKeyPress
    end
    object UpDownWidth: TUpDown
      Left = 177
      Top = 43
      Width = 17
      Height = 21
      Associate = EditWidth
      Min = 4
      Max = 4096
      Increment = 4
      Position = 32
      TabOrder = 4
      Thousands = False
    end
    object UpDownHeight: TUpDown
      Left = 235
      Top = 43
      Width = 17
      Height = 21
      Associate = EditHeight
      Min = 4
      Max = 4096
      Increment = 4
      Position = 30
      TabOrder = 5
      Thousands = False
    end
    object RadioButtonNormal: TRadioButton
      Left = 16
      Top = 24
      Width = 105
      Height = 17
      Caption = 'Standard 32x30'
      Checked = True
      TabOrder = 6
      TabStop = True
      OnClick = RadioButtonNormalClick
    end
    object RadioButtonUser: TRadioButton
      Left = 16
      Top = 47
      Width = 105
      Height = 17
      Caption = 'User defined'
      TabOrder = 7
      OnClick = RadioButtonNormalClick
    end
  end
end
