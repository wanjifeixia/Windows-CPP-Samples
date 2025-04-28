/* 作者：LESLIEXIN
 * 邮箱：LESLIEXIN@OUTLOOK.COM
 * 网站：HTTP://WWW.LESLIEXIN.COM/
 * 版权所有，使用时需保留此段作者信息。
 * 
 */
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace DragDropDemo
{
    public partial class Form3 : Form
    {
        public Form3()
        {
            InitializeComponent();
        }


        Image ImgDeal(Image img,byte alpha,int width,int height)
        {
            var bmpSize = new Bitmap(width, height);
            using (var g = Graphics.FromImage(bmpSize))
            {
                g.DrawImage(img, new Rectangle(0,0,width,height));
            }
            var bmpAlpha = new Bitmap(width,height);
            for (int i = 0; i < width; i++)
            {
                for (int j = 0; j < height; j++)
                {
                    var color = bmpSize.GetPixel(i, j);
                    bmpAlpha.SetPixel(i, j, Color.FromArgb(alpha, color));
                }
            }
            return (Image)bmpAlpha.Clone();
        }

        private Rectangle dragBox;

        private void pictureBox1_MouseDown(object sender, MouseEventArgs e)
        {
            dragBox = new Rectangle(new Point(e.X - (SystemInformation.DragSize.Width / 2),
               e.Y - (SystemInformation.DragSize.Height / 2)), SystemInformation.DragSize);
        }

        private void pictureBox1_MouseMove(object sender, MouseEventArgs e)
        {
            if ((e.Button & MouseButtons.Left) == MouseButtons.Left)
            {
                if (dragBox != Rectangle.Empty &&
                    !dragBox.Contains(e.X, e.Y))
                {
                    var effect = pictureBox1.DoDragDrop(pictureBox1.Image, DragDropEffects.All | DragDropEffects.Link);
                    if (effect == DragDropEffects.Move)
                        pictureBox1.Image = null;
                }
            }
        }

        private void pictureBox1_MouseUp(object sender, MouseEventArgs e)
        {
            dragBox = Rectangle.Empty;
        }

        private void pictureBox1_GiveFeedback(object sender, GiveFeedbackEventArgs e)
        {
            e.UseDefaultCursors = false;
            if (e.Effect == DragDropEffects.None)
            {
                var img=ImgDeal(pictureBox1.Image, 70, pictureBox1.Width, pictureBox1.Height);
                Cursor.Current = new Cursor(((Bitmap)img).GetHicon());
            }
            else
            {
                var img = ImgDeal(pictureBox1.Image, 255, pictureBox1.Width, pictureBox1.Height);
                Cursor.Current = new Cursor(((Bitmap)img).GetHicon());
            }
        }


        private void panel1_DragEnter(object sender, DragEventArgs e)
        {
            if (!e.Data.GetDataPresent(typeof(Bitmap)))
            {
                e.Effect = DragDropEffects.None;
            }
        }

        private void panel1_DragOver(object sender, DragEventArgs e)
        {
            if ((e.AllowedEffect & DragDropEffects.None) == DragDropEffects.None
                && (e.KeyState & (8 + 32)) == (8 + 32))
            {
                //CTRL+ALT
                e.Effect = DragDropEffects.None;
                label2.Text = $"按键状态：CTRL+ALT\r\n效果：None";
            }
            else if ((e.AllowedEffect & DragDropEffects.Link) == DragDropEffects.Link
                && (e.KeyState & (32)) == (32))
            {
                //ALT
                e.Effect = DragDropEffects.Link;
                label2.Text = $"按键状态：ALT\r\n效果：Link";
            }
            else if ((e.AllowedEffect & DragDropEffects.Copy) == DragDropEffects.Copy
                && (e.KeyState & (8)) == (8))
            {
                //CTRL
                e.Effect = DragDropEffects.Copy;
                label2.Text = $"按键状态：CTRL+ALT\r\n效果：Copy";
            }
            else if ((e.AllowedEffect & DragDropEffects.Move) == DragDropEffects.Move
                && (e.KeyState & (4)) == (4))
            {
                //SHIFT
                e.Effect = DragDropEffects.Move;
                label2.Text = $"按键状态：SHIFT\r\n效果：Move";
            }
            else if ((e.AllowedEffect & DragDropEffects.Copy) == DragDropEffects.Copy)
            {
                //无
                e.Effect = DragDropEffects.Copy;
                label2.Text = $"按键状态：无\r\n效果：Copy";
            }
            else
            {
                e.Effect = DragDropEffects.None;
                label2.Text = $"按键状态：无\r\n效果：None";
            }
        }

        private void panel1_DragDrop(object sender, DragEventArgs e)
        {
            panel1.BackgroundImage = (Bitmap)e.Data.GetData(typeof(Bitmap));
        }


        private void Form3_Load(object sender, EventArgs e)
        {
            pictureBox1.Image = Resource1.logo_x360;
        }

        private void panel1_Click(object sender, EventArgs e)
        {
            panel1.BackgroundImage = null;
        }
    }
}
