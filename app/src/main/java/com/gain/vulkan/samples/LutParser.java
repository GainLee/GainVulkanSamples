/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Gain
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
package com.gain.vulkan.samples;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.os.Build;
import android.util.Log;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;

public class LutParser {
    
    private static final String TAG = "LutParser";

    public static Bitmap parse(File file){
        String suffix = getFileSuffix(file);
        if (suffix.contains("CUBE") || suffix.contains("cube")) {
            return encodeCube(file);
        } else if (suffix.contains("3DL") || suffix.contains("3dl")) {
            return encode3dl(file);
        } else {
            Log.e(TAG,"not support format");
            return null;
        }
    }

    public static Bitmap encodeCube(File file) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            try {
                Log.i(TAG, "encodeCube:  start");
                int lutSize = 0;
                BufferedReader br = new BufferedReader(new FileReader(file));
                String line;
                String[] parts;
                int index = 0;
                Bitmap bitmap = null;
                float[] minValue = {0.0f,0.0f,0.0f};
                float[] maxValue = {1.0f,1.0f,1.0f};

                while ((line = br.readLine()) != null) {
                    if (line.startsWith("#")) {
                        continue;
                    }
                    if (line.isEmpty()) {
                        continue;
                    }
                    parts = line.split(" ");
                    if (parts.length == 2) {
                        if (parts[0].equalsIgnoreCase("LUT_3D_SIZE")) {
                            lutSize = Integer.parseInt(parts[1]);
                            if (bitmap == null) {
                                bitmap = Bitmap.createBitmap(lutSize,lutSize * lutSize,Bitmap.Config.ARGB_8888);
                                continue;
                            } else {
                                Log.i(TAG, "corrupted data");
                                break;
                            }
                        }
                    }

                    if (parts.length != 3) {
                        if (parts[0].equalsIgnoreCase("DOMAIN_MIN")) {
                            minValue[0] = Float.parseFloat(parts[1]);
                            minValue[1] = Float.parseFloat(parts[2]);
                            minValue[2] = Float.parseFloat(parts[3]);
                            Log.i(TAG, "DOMAIN_MIN " + minValue[0] + " " + minValue[1] + " " + minValue[2]);
                        } else if (parts[0].equalsIgnoreCase("DOMAIN_MAX")) {
                            maxValue[0] = Float.parseFloat(parts[1]);
                            maxValue[1] = Float.parseFloat(parts[2]);
                            maxValue[2] = Float.parseFloat(parts[3]);
                            Log.i(TAG, "DOMAIN_MAX " + maxValue[0] + " " + maxValue[1] + " " + maxValue[2]);
                        } else {
                            Log.i(TAG, "unknown data: " + line);
                        }
                        continue;
                    }
                    if (bitmap == null) {
                        Log.i(TAG, "not ready yet");
                        continue;
                    }

                    float r = (Float.parseFloat(parts[0]) - minValue[0]) / (maxValue[0] - minValue[0]);
                    float g = (Float.parseFloat(parts[1]) - minValue[1]) / (maxValue[1] - minValue[1]);
                    float b = (Float.parseFloat(parts[2]) - minValue[2]) / (maxValue[2] - minValue[2]);
                    int x = index % lutSize;
                    int y = index / lutSize;
                    int argb = Color.argb(1.0f, r, g, b);
                    bitmap.setPixel(x, y, argb);
                    index++;

                }

                int widthdim = lutSize;
                int heightdim = lutSize * lutSize;

                Log.i(TAG, "dimension " + widthdim + "x" + heightdim);
                Log.i(TAG, "encodeCube:  end");
                return bitmap;
            } catch (IOException e) {
                Log.i(TAG, e.getMessage());
                e.printStackTrace();
                return null;
            }
        } else {
            return null;
        }
    }

    public static Bitmap encode3dl(File file) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            try {
                Log.i(TAG, "encode3dl:  start");
                int lutSize = 0;
                BufferedReader br = new BufferedReader(new FileReader(file));
                String line;
                String[] parts;
                Bitmap bitmap = null;
                int index = 0;
                float scale = 4096.0f;
                while ((line = br.readLine()) != null) {
                    if (line.startsWith("#")) {
                        continue;
                    }
                    if (line.isEmpty()) {
                        continue;
                    }
                    parts = line.split(" ");

                    if (line.startsWith("Mesh")) {
                        int bits = Integer.parseInt(parts[2]);
                        scale = (float) Math.pow(2,bits);
                        Log.i(TAG, " Mesh scale:" + scale + " bits:" + bits);
                        continue;
                    }

                    if (parts.length != 3) {
                        if (parts.length > 3 ) {
                            lutSize = parts.length;
                            Log.i(TAG, "lutSize:" + lutSize);

                            if (bitmap == null) {
                                bitmap = Bitmap.createBitmap(lutSize,lutSize * lutSize,Bitmap.Config.ARGB_8888);
                                continue;
                            } else {
                                Log.i(TAG,"corrupted data");
                                break;
                            }
                        }
                        continue;
                    }
                    if (bitmap == null) {
                        Log.i(TAG, "not ready yet");
                        continue;
                    }

                    float r = Float.parseFloat(parts[0])/scale;
                    float g = Float.parseFloat(parts[1])/scale;
                    float b = Float.parseFloat(parts[2])/scale;

                    int squareIndex = index % lutSize;
                    int inSquareY = (index / lutSize) % lutSize;
                    int inSquareX = index / lutSize / lutSize;

                    int x = inSquareX;
                    int y = squareIndex * lutSize + inSquareY;
                    int argb = Color.argb(1.0f, r, g, b);
                    bitmap.setPixel(x, y, argb);
                    index++;
                }

                int widthdim = lutSize;
                int heightdim = lutSize* lutSize;

                Log.i(TAG, "dimension " + widthdim + "x" + heightdim);

                Log.i(TAG, "encode3dl:  end");
                return bitmap;

            } catch (IOException e) {
                Log.i(TAG, e.getMessage());
                e.printStackTrace();
                return null;
            }
        } else {
            return null;
        }

    }

    public static void saveBitmap(Bitmap bmp, String picName) {
        Log.i("LutParser", "saveBitmap start ");
        File f = new File(picName);
        if (f.exists()) {
            f.delete();
        }
        FileOutputStream out = null;
        try {
            out = new FileOutputStream(f);
            bmp.compress(Bitmap.CompressFormat.PNG, 90, out);
            out.flush();
        } catch (FileNotFoundException e) {
            Log.e(TAG, " FileNotFoundException ");
            e.printStackTrace();
        } catch (IOException e) {
            Log.e(TAG, " IOException");
            e.printStackTrace();
        } finally {
            if (out != null) {
                try {
                    out.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
        Log.i(TAG, "saveBitmap:  " +picName);
    }


    public static String getFileSuffix(File file){
        String fileName = file.getName();
        String suffix = fileName.substring(fileName.lastIndexOf(".") + 1);
        return suffix;

    }

    public static Bitmap createBitmapFromFile(String path, boolean isAssert, Context context) {
        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inScaled = false;
        InputStream inputStream = null;
        FileInputStream fileInput = null;
        Bitmap bitmap = null;
        try{
            if (isAssert) {
                inputStream = context.getAssets().open(path);
            } else {
                fileInput = new FileInputStream(path);
                inputStream = new BufferedInputStream(fileInput);
            }
            bitmap = BitmapFactory.decodeStream(inputStream);
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            if (fileInput!= null) {
                try {
                    fileInput.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
            if (inputStream!= null) {
                try {
                    inputStream.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }

            }

        }

        return bitmap;
    }

}
