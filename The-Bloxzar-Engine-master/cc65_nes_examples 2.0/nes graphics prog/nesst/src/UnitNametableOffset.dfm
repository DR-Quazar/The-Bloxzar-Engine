object FormNameOffset: TFormNameOffset
  Left = 0
  Top = 0
  BorderIcons = []
  BorderStyle = bsSingle
  Caption = 'offset/replace'
  ClientHeight = 184
  ClientWidth = 243
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -13
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  Position = poMainFormCenter
  Scaled = False
  OnCreate = FormCreate
  OnShow = FormShow
  PixelsPerInch = 96
  TextHeight = 16
  object GroupBox1: TGroupBox
    Left = 0
    Top = 0
    Width = 243
    Height = 184
    Align = alClient
    TabOrder = 0
    object LabelFromTo: TLabel
      Left = 96
      Top = 27
      Width = 33
      Height = 16
      Alignment = taRightJustify
      Caption = 'range'
      Layout = tlBottom
    end
    object LabelOffset: TLabel
      Left = 97
      Top = 69
      Width = 32
      Height = 16
      Alignment = taRightJustify
      Caption = 'offset'
    end
    object LabelInfo: TLabel
      Left = 12
      Top = 104
      Width = 22
      Height = 16
      Caption = 'Info'
    end
    object ButtonOk: TButton
      Left = 77
      Top = 148
      Width = 75
      Height = 25
      Caption = 'Ok'
      TabOrder = 0
      OnClick = ButtonOkClick
    end
    object ButtonCancel: TButton
      Left = 158
      Top = 148
      Width = 75
      Height = 25
      Caption = 'Cancel'
      TabOrder = 1
      OnClick = ButtonCancelClick
    end
    object EditFrom: TEdit
      Left = 135
      Top = 24
      Width = 42
      Height = 24
      MaxLength = 4
      TabOrder = 2
      Text = '0'
      OnChange = EditFromChange
      OnKeyPress = EditFromKeyPress
      OnMouseDown = EditFromMouseDown
    end
    object EditTo: TEdit
      Left = 191
      Top = 24
      Width = 42
      Height = 24
      MaxLength = 4
      TabOrder = 3
      Text = '255'
      OnChange = EditFromChange
      OnKeyPress = EditFromKeyPress
      OnMouseDown = EditFromMouseDown
    end
    object EditOffset: TEdit
      Left = 135
      Top = 66
      Width = 42
      Height = 24
      MaxLength = 4
      TabOrder = 4
      Text = '0'
      OnChange = EditFromChange
      OnKeyPress = EditFromKeyPress
      OnMouseDown = EditFromMouseDown
    end
  end
end
