//
//  Shader.fsh
//  Benchmarks
//
//  Created by Siarhei Rachytski on 4/11/11.
//  Copyright 2011 Credo-Dialogue. All rights reserved.
//

varying lowp vec4 colorVarying;

void main()
{
    gl_FragColor = colorVarying;
}
