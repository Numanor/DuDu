<template>
  <div>
    <baidu-map :center="psg" :zoom="zoom" @ready="initCenter" id="map" :scroll-wheel-zoom="true">
      <bm-polyline :path="drivePath" stroke-color="blue" :stroke-opacity="0.6" :stroke-weight="5"></bm-polyline>
      <bm-marker v-for="(cab, index) in cabs" :key="index" :position="cab.pos" :dragging="false" :icon="icon.cab" :title="cab.id"></bm-marker>
      <bm-marker :position="psg" :dragging="false" animation="BMAP_ANIMATION_BOUNCE" :icon="icon.psg"></bm-marker>
      <bm-marker :position="dest" :dragging="false" animation="BMAP_ANIMATION_DROP" :icon="icon.dest"></bm-marker>
    </baidu-map>
    <el-row style="padding:25px 0px; background:rgba(200,200,200,0.8)">
      <el-col :span="10">
        <span>Location</span>
        <el-input-number v-model="psg.lng" :precision="3" :step="0.001" :max="135" :min="73"></el-input-number>
        <el-input-number v-model="psg.lat" :precision="3" :step="0.001" :max="53" :min="4"></el-input-number>
      </el-col>
      <el-col :span="10">
        <span>Destination</span>
        <el-input-number v-model="dest.lng" :precision="3" :step="0.001" :max="135" :min="73"></el-input-number>
        <el-input-number v-model="dest.lat" :precision="3" :step="0.001" :max="53" :min="4"></el-input-number>
      </el-col>
      <el-col :span="2">
        <el-button type="primary" icon="el-icon-search" @click="setPsg" round>Search</el-button>
      </el-col>
    </el-row>
    <el-row>
      <el-col :span="6.5">
        <el-table
          :data="cabs"
          :default-sort = "{prop: 'len', order: 'ascending'}"
          highlight-current-row
          @current-change="changeFocus"
          style="width: 100%;">
          <el-table-column
            prop="id"
            label="ID"
            width="80">
          </el-table-column>
          <el-table-column
            prop="away"
            label="From you(m)"
            sortable
            width="140">
          </el-table-column>
          <el-table-column
            prop="psg_num"
            label="Passengers"
            sortable
            width="120">
          </el-table-column>
          <el-table-column
            prop="len"
            label="Trip takes(m)"
            sortable
            width="140">
          </el-table-column>
        </el-table>
      </el-col>
    </el-row>
  </div>
</template>
<script>
import axios from 'axios'
export default {
  name: 'CabMap',
  data () {
    return {
      zoom: 3,
      icon: {
        psg: {
          url: require('../assets/iconfinder_taxi_passanger_41989.png'),
          size: {width: 50, height: 80},
          opts: {
            imageSize: {width: 50, height: 50}
          }
        },
        cab: {
          url: require('../assets/iconfinder_Cab_3363563.png'),
          size: {width: 50, height: 80},
          opts: {
            imageSize: {width: 50, height: 50}
          }
        },
        dest: {
          url: require('../assets/iconfinder_pin-destination-map-location_2075808.png'),
          size: {width: 20, height: 62},
          opts: {
            imageSize: {width: 20, height: 31}
          }
        }
      },
      psg: {lng: 116.404, lat: 39.915},
      dest: {lng: 116.404, lat: 39.915},
      cabs: [],
      focusCab: -1,
    }
  },
  methods: {
    initCenter ({BMap, map}) {
      console.log(BMap, map)
      this.psg.lng = 116.404
      this.psg.lat = 39.915
      this.zoom = 17
      // this.axios.default
    },
    setPsg () {
      // this.changeFocus(this.focusCab)
      this.getCabs()
    },
    getCabs () {
      this.cabs = []
      var param = {
        psg_lng: this.psg.lng,
        psg_lat: this.psg.lat,
        dst_lng: this.dest.lng,
        dst_lat: this.dest.lat
      }
      console.log(param)
      let _this = this;
      this.$http.post('/api/add_json', JSON.stringify(param)).then(function (response) {
        console.log(response.data)
        if (response.data.cab.length > 0) {
          _this.$notify({
            title: 'Success',
            message: (response.data.cab.length+' available cabs answers!'),
            type: 'success'
          })
          _this.dest = response.data.T
          _this.psg = response.data.S
          _this.cabs = response.data.cab
          for (var i=0; i!=_this.cabs.length; ++i) {
            _this.cabs[i].pos = _this.cabs[i].path[0]
          }
        } else {
          _this.$notify({
            title: 'Oops...',
            message: 'No available cab was found.',
            type: 'warning'
          })
        }
      }).catch(function (error) {
        console.log(error);
      });
    },
    // sleep (numberMillis) {
    //   var now = new Date();
    //   var exitTime = now.getTime() + numberMillis;
    //   while (true) {
    //       now = new Date();
    //       if (now.getTime() > exitTime)
    //           return;
    //   }
    // },
    changeFocus (currentRow, oldRow) {
      var newFocus
      for (var i=0; i!=this.cabs.length; ++i) {
        if (currentRow.id == this.cabs[i].id) {
          newFocus = i
          break
        }
      }
      if (newFocus != this.focusCab) {
        this.focusCab = newFocus
      } else {
        this.focusCab = -1
      }
    }
  },
  computed: {
    drivePath: function () {
      if (this.focusCab == -1) {
        return null
      }
      return this.cabs[this.focusCab].path
    }
  }
}
</script>

<!-- Add "scoped" attribute to limit CSS to this component only -->
<style scoped>
#map {
  position: fixed;
  width: 100%;
  height: 100vh;
  z-index: -1;
}
span {
  color: #409EFF;
  font-weight: bolder;
  font-size: 30px;
  margin-right: 20px;
}
.el-table {
  background-color: rgba(255,255,255,0.3)
}
</style>
