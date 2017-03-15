object FormBank: TFormBank
  Left = 0
  Top = 0
  BorderStyle = bsDialog
  Caption = 'FormBank'
  ClientHeight = 340
  ClientWidth = 542
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
  object GroupBoxInfo: TGroupBox
    Left = 0
    Top = 0
    Width = 542
    Height = 340
    Align = alClient
    TabOrder = 0
    ExplicitWidth = 281
    ExplicitHeight = 216
    object ImageCHR1: TImage
      Left = 8
      Top = 38
      Width = 256
      Height = 256
    end
    object ImageCHR2: TImage
      Left = 278
      Top = 38
      Width = 256
      Height = 256
    end
    object LabelInfo: TLabel
      Left = 8
      Top = 15
      Width = 52
      Height = 16
      Caption = 'LabelInfo'
    end
    object Label1: TLabel
      Left = 8
      Top = 311
      Width = 32
      Height = 16
      Caption = 'Bank:'
    end
    object ButtonOK: TButton
      Left = 407
      Top = 308
      Width = 61
      Height = 25
      Caption = 'OK'
      Enabled = False
      TabOrder = 0
      OnClick = ButtonOKClick
    end
    object ButtonCancel: TButton
      Left = 474
      Top = 308
      Width = 60
      Height = 25
      Caption = 'Cancel'
      TabOrder = 1
      OnClick = ButtonCancelClick
    end
    object EditBank: TEdit
      Left = 52
      Top = 308
      Width = 41
      Height = 24
      TabOrder = 2
      Text = '0'
      OnChange = EditBankChange
    end
    object UpDownBank: TUpDown
      Left = 93
      Top = 308
      Width = 19
      Height = 24
      Associate = EditBank
      TabOrder = 3
    end
  end
end
