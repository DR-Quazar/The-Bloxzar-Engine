object FormCHREditor: TFormCHREditor
  Left = 0
  Top = 0
  BorderIcons = [biSystemMenu]
  BorderStyle = bsToolWindow
  Caption = 'CHR Editor'
  ClientHeight = 333
  ClientWidth = 281
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -13
  Font.Name = 'Tahoma'
  Font.Style = []
  FormStyle = fsStayOnTop
  OldCreateOrder = False
  Position = poMainFormCenter
  Scaled = False
  OnCreate = FormCreate
  OnKeyDown = FormKeyDown
  OnMouseWheel = FormMouseWheel
  PixelsPerInch = 96
  TextHeight = 16
  object GroupBox1: TGroupBox
    Left = 0
    Top = 0
    Width = 281
    Height = 289
    Align = alTop
    TabOrder = 0
    object PaintBoxChr: TPaintBox
      Left = 4
      Top = 11
      Width = 272
      Height = 272
      OnMouseDown = PaintBoxChrMouseDown
      OnMouseMove = PaintBoxChrMouseMove
      OnPaint = PaintBoxChrPaint
    end
  end
  object GroupBox2: TGroupBox
    Left = 0
    Top = 289
    Width = 281
    Height = 44
    Align = alBottom
    TabOrder = 1
    object SpeedButtonHFlip: TSpeedButton
      Left = 10
      Top = 16
      Width = 41
      Height = 22
      Hint = 'Flip sprite graphics horizontally'
      Caption = 'HFlip'
      ParentShowHint = False
      ShowHint = True
      OnClick = SpeedButtonHFlipClick
    end
    object SpeedButtonVFlip: TSpeedButton
      Left = 56
      Top = 16
      Width = 41
      Height = 22
      Hint = 'Flip sprite graphics vertically'
      Caption = 'VFlip'
      ParentShowHint = False
      ShowHint = True
      OnClick = SpeedButtonVFlipClick
    end
    object SpeedButtonRotateCCW: TSpeedButton
      Left = 101
      Top = 16
      Width = 44
      Height = 22
      Hint = 'Rotate sprite graphics 90 degree counter-clockwise'
      Caption = 'CCW'
      ParentShowHint = False
      ShowHint = True
      OnClick = SpeedButtonRotateCCWClick
    end
    object SpeedButtonRotateCW: TSpeedButton
      Left = 150
      Top = 16
      Width = 44
      Height = 22
      Hint = 'Rotate sprite graphics 90 degree clockwise'
      Caption = 'CW'
      ParentShowHint = False
      ShowHint = True
      OnClick = SpeedButtonRotateCWClick
    end
  end
end
